#include "tracking.h"
#include "motor.h"
#include "grayscale.h"
#include "mpu6050.h"
#include "lcd.h"
#include "delay.h"
#include "debug.h"

/* ---- 陀螺仪参数 ---- */
#define GYRO_SCALE     0.01527f
#define INTEG_THRESH   1.0f
#define LOST_ANGLE     30.0f
#define LOST_KP        4.0f
#define LOST_PWM_MIN   80
#define LOST_PWM_MAX   250

/* ---- 内部状态 ---- */
uint8_t current_mode = MODE_STOP;

static float    gyro_bias;
static float    yaw_angle = 0.0f;
static int16_t  SPD_BASE   = 200;
static int16_t  SPD_SLIGHT = 75;
static int16_t  SPD_STRONG = 150;
static uint8_t  gs_val;
static uint8_t  lost_flag = 0;
static uint8_t  last_seen = 0;
static float    lost_yaw = 0.0f;
static float    target_offset = 0.0f;

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
}

void Tracking_Gyro_Calibrate(void)
{
    int32_t sum = 0;
    uint16_t i;

    LCD_ShowString(280, 220, "Calibrating gyro...", YELLOW, BLACK, 16);
    LCD_ShowString(330, 245, "Keep still!", RED, BLACK, 16);

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
    uint8_t SL, LM, M, RM, SR;

    (void)dt_us;

    if (current_mode == MODE_STOP) {
        Motor_Stop();
        lost_flag = 0;
        return;
    }

    /* 读取灰度传感器 */
    gs_val = Grayscale_Read();
    SL = (gs_val >> 4) & 1;
    LM = (gs_val >> 3) & 1;
    M  = (gs_val >> 2) & 1;
    RM = (gs_val >> 1) & 1;
#if SR_BROKEN
    SR = 0;
#else
    SR = (gs_val >> 0) & 1;
#endif

    if (SL || LM || M || RM || SR) {
        /* 检测到线：加权平均循迹 */
        lost_flag = 0;
        last_seen = (SL << 4) | (LM << 3) | (M << 2) | (RM << 1) | SR;

        /* 加权偏差: SL=-2, LM=-1, M=0, RM=+1, SR=+2 */
        int8_t error = -2 * SL - 1 * LM + 0 * M + 1 * RM + 2 * SR;

        pwm_l = SPD_BASE + error * SPD_STRONG;
        pwm_r = SPD_BASE - error * SPD_STRONG;

        Motor_SetSpeed(-pwm_l, -pwm_r);

    } else {
        /* 脱线：MPU6050 辅助恢复 */
        if (!lost_flag) {
            uint8_t last_SL = (last_seen >> 4) & 1;
            uint8_t last_LM = (last_seen >> 3) & 1;
            uint8_t last_RM = (last_seen >> 1) & 1;
            uint8_t last_SR = last_seen & 1;

            lost_flag = 1;
            lost_yaw = yaw_angle;

            if (last_SL || last_LM) {
                target_offset = LOST_ANGLE;
            } else if (last_SR || last_RM) {
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

/* ---- 状态查询 ---- */
float   Tracking_GetYaw(void)       { return yaw_angle; }
uint8_t Tracking_GetGS(void)        { return gs_val; }
uint8_t Tracking_GetLost(void)      { return lost_flag; }

/* ---- 参数读写 ---- */
void    Tracking_SetBaseSpeed(int16_t v)    { SPD_BASE = v; }
void    Tracking_SetSlightSpeed(int16_t v)  { SPD_SLIGHT = v; }
void    Tracking_SetStrongSpeed(int16_t v)  { SPD_STRONG = v; }
int16_t Tracking_GetBaseSpeed(void)         { return SPD_BASE; }
int16_t Tracking_GetSlightSpeed(void)       { return SPD_SLIGHT; }
int16_t Tracking_GetStrongSpeed(void)       { return SPD_STRONG; }
