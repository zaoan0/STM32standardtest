#include "tracking.h"
#include "motor.h"
#include "grayscale.h"
#include "mpu6050.h"
#include "lcd.h"
#include "delay.h"
#include "debug.h"

/* ---- Gyro 参数 ---- */
#define GYRO_SCALE     0.01527f
#define INTEG_THRESH   1.0f
#define LOST_ANGLE     30.0f
#define LOST_KP        4.0f
#define LOST_PWM_MIN   80
#define LOST_PWM_MAX   250

/* ---- 8路 Sensor 阈值（调试后调整） ---- */
#define GS_THRESHOLD   2000

/* ---- 内部状态 ---- */
uint8_t current_mode = MODE_STOP;

static float    gyro_bias;
static float    yaw_angle = 0.0f;
static int16_t  SPD_BASE   = 200;
static int16_t  SPD_SLIGHT = 50;
static int16_t  SPD_STRONG = 100;
static uint16_t gs_raw[GS_CHANNELS];   /* 原始ADC值（0-4095） */
static uint8_t  lost_flag = 0;
static uint8_t  last_seen = 0;
static float    lost_yaw = 0.0f;
static float    target_offset = 0.0f;

/* ---- Gyro 直线状态 ---- */
#define GYRO_STRAIGHT_KP  5.0f

static uint8_t  gyro_straight_running = 0;
static float    gyro_straight_target  = 0.0f;
static int16_t  gyro_straight_speed   = 200;
static int16_t  gyro_straight_output  = 0;

/* ---- 工具函数 ---- */
static int16_t clamp_pwm(int16_t v)
{
    if (v >  LOST_PWM_MAX) return  LOST_PWM_MAX;
    if (v < -LOST_PWM_MAX) return -LOST_PWM_MAX;
    if (v > 0 && v < LOST_PWM_MIN) return  LOST_PWM_MIN;
    if (v < 0 && v > -LOST_PWM_MIN) return -LOST_PWM_MIN;
    return v;
}

/* ========== 公共接口 ========== */

void Tracking_Init(void)
{
    Motor_Init();
    Debug_Printf("Motor OK\r\n");
    Grayscale_Init();
    Debug_Printf("Grayscale OK\r\n");
    MPU6050_Init();
    Debug_Printf("MPU6050 OK\r\n");

    if (MPU6050_Check()) {
        Debug_Printf("MPU6050 detected\r\n");
    } else {
        Debug_Printf("ERROR: MPU6050 not found! Check PB1(SCL)/PB2(SDA)\r\n");
    }
}

void Tracking_Gyro_Calibrate(void)
{
    int32_t sum = 0;
    uint16_t i;

    LCD_ShowMixedString(280, 220, "������У׼��...", YELLOW, BLACK);
    LCD_ShowMixedString(330, 245, "�뱣�־�ֹ!", RED, BLACK);

    for (i = 0; i < 500; i++) {
        sum += MPU6050_ReadGyroZ();
        delay_us(2500);
    }
    gyro_bias = sum / 500.0f;

    LCD_FillRect(280, 220, 300, 50, BLACK);
    Debug_Printf("Gyro bias: %d\r\n", (int)gyro_bias);
}

void Tracking_UpdateGyro(uint32_t dt_us)
{
    int16_t gz_raw = MPU6050_ReadGyroZ();
    float yaw_rate = (gz_raw - gyro_bias) * GYRO_SCALE;

    if (yaw_rate > INTEG_THRESH || yaw_rate < -INTEG_THRESH) {
        yaw_angle += yaw_rate * ((float)dt_us / 1000000.0f);
    }
}

void Tracking_Run(uint32_t dt_us)
{
    int16_t pwm_l, pwm_r, pwm;
    float error;
    uint8_t any_on = 0;
    uint8_t i;

    (void)dt_us;

    if (current_mode == MODE_STOP) {
        Motor_Stop();
        lost_flag = 0;
        return;
    }

    /* 读取全部8路通道 */
    Grayscale_ReadAll(gs_raw);

    /* 加权平均：CH1=-3.5 ... CH8=+3.5
     * 权重：-7, -5, -3, -1, +1, +3, +5, +7（x2避免浮点）
     * 模拟量：on_track = (GS_THRESHOLD - raw) / GS_THRESHOLD (0~1)
     * 黑线 = 低ADC值
     */
    {
        int32_t weighted_sum = 0;
        int32_t weight_norm = 0;
        static const int8_t weights[8] = {7, 5, 3, 1, -1, -3, -5, -7};

        for (i = 0; i < GS_CHANNELS; i++) {
            if (gs_raw[i] < GS_THRESHOLD) {
                /* 在线上：强度 = 深度 (0=白, GS_THRESHOLD=黑) */
                int32_t strength = GS_THRESHOLD - gs_raw[i];
                weighted_sum += weights[i] * strength;
                weight_norm += strength;
                any_on = 1;
            }
        }

        if (any_on && weight_norm > 0) {
            error = (float)weighted_sum / (float)weight_norm;
        } else {
            error = 0.0f;
        }
    }

    if (any_on) {
        /* 检测到线：加权循迹 */
        lost_flag = 0;
        last_seen = 0;
        for (i = 0; i < GS_CHANNELS; i++) {
            if (gs_raw[i] < GS_THRESHOLD) last_seen |= (1 << i);
        }

        pwm_l = SPD_BASE + (int16_t)(error * SPD_STRONG);
        pwm_r = SPD_BASE - (int16_t)(error * SPD_STRONG);

        Motor_SetSpeed(-pwm_l, -pwm_r);

    } else {
        /* 脱线：Gyro 辅助恢复 */
        if (!lost_flag) {
            lost_flag = 1;
            lost_yaw = yaw_angle;

            /* 根据最后看到的 Sensor 确定恢复方向 */
            if (last_seen & 0xC0) {        /* CH7, CH8 (左侧) */
                target_offset = LOST_ANGLE;
            } else if (last_seen & 0x03) { /* CH1, CH2 (右侧) */
                target_offset = -LOST_ANGLE;
            } else {
                target_offset = 0.0f;
            }
        }

        if (target_offset == 0.0f) {
            Motor_SetSpeed(-SPD_BASE, -SPD_BASE);
        } else {
            float yaw_change = yaw_angle - lost_yaw;
            float err = target_offset - yaw_change;
            pwm = clamp_pwm((int16_t)(LOST_KP * err));

            if (target_offset > 0) {
                pwm_l = SPD_BASE - pwm;
                pwm_r = SPD_BASE + pwm;
            } else {
                pwm_l = SPD_BASE + pwm;
                pwm_r = SPD_BASE - pwm;
            }

            if (pwm_l < 0) pwm_l = 0;
            if (pwm_r < 0) pwm_r = 0;

            Motor_SetSpeed(-pwm_l, -pwm_r);
        }
    }
}

/* ---- Gyro 直线 ---- */
void Tracking_GyroStraight_Start(float target_angle, int16_t speed)
{
    gyro_straight_target  = target_angle;
    gyro_straight_speed   = speed;
    gyro_straight_running = 1;
    gyro_straight_output  = 0;
    yaw_angle = 0.0f;
    Debug_Printf("GyroStraight START: target=%d speed=%d\r\n",
                 (int)target_angle, speed);
}

void Tracking_GyroStraight_Stop(void)
{
    gyro_straight_running = 0;
    gyro_straight_output  = 0;
    Motor_Stop();
}

void Tracking_GyroStraight_Run(uint32_t dt_us)
{
    float err;
    int16_t pwm_corr;
    int16_t pwm_l, pwm_r;
    static uint32_t dbg_tick = 0;

    if (!gyro_straight_running) {
        Motor_Stop();
        return;
    }

    err = gyro_straight_target - yaw_angle;
    pwm_corr = (int16_t)(GYRO_STRAIGHT_KP * err);

    if (pwm_corr >  200) pwm_corr =  200;
    if (pwm_corr < -200) pwm_corr = -200;

    pwm_l = gyro_straight_speed + pwm_corr;
    pwm_r = gyro_straight_speed - pwm_corr;

    if (pwm_l < 0) pwm_l = 0;
    if (pwm_r < 0) pwm_r = 0;
    if (pwm_l > PWM_MAX) pwm_l = PWM_MAX;
    if (pwm_r > PWM_MAX) pwm_r = PWM_MAX;

    gyro_straight_output = pwm_corr;

    dbg_tick += dt_us;
    if (dbg_tick >= 500000) {
        dbg_tick = 0;
        Debug_Printf("GS: yaw=%d err=%d corr=%d\r\n",
                     (int)(yaw_angle * 10), (int)(err * 10), pwm_corr);
    }

    Motor_SetSpeed(-pwm_l, -pwm_r);
}

uint8_t Tracking_GyroStraight_IsRunning(void)
{
    return gyro_straight_running;
}

int16_t Tracking_GyroStraight_GetOutput(void)
{
    return gyro_straight_output;
}

/* ---- 状态查询 ---- */
float   Tracking_GetYaw(void)       { return yaw_angle; }
uint8_t Tracking_GetGS(void)        { return 0; }  /* 已弃用，使用raw */
uint8_t Tracking_GetLost(void)      { return lost_flag; }
const uint16_t *Tracking_GetGSRaw(void)
{
    Grayscale_ReadAll(gs_raw);
    return gs_raw;
}

/* ---- 参数访问 ---- */
void    Tracking_SetBaseSpeed(int16_t v)    { SPD_BASE = v; }
void    Tracking_SetSlightSpeed(int16_t v)  { SPD_SLIGHT = v; }
void    Tracking_SetStrongSpeed(int16_t v)  { SPD_STRONG = v; }
int16_t Tracking_GetBaseSpeed(void)         { return SPD_BASE; }
int16_t Tracking_GetSlightSpeed(void)       { return SPD_SLIGHT; }
int16_t Tracking_GetStrongSpeed(void)       { return SPD_STRONG; }
