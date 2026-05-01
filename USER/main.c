#include "stm32f10x.h"
#include "motor.h"
#include "grayscale.h"
#include "mpu6050.h"
#include "delay.h"

/*
 * v4: 循迹 + MPU6050 脱线恢复
 * - 正常：灰度传感器循迹
 * - 脱线：记录 yaw，按上次传感器方向转固定角度找线
 *   - 直行丢失 → 保持直行
 *   - 左侧丢失 → 左转寻线
 *   - 右侧丢失 → 右转寻线
 * - 找到线 → 恢复正常循迹
 */

#define SPD_BASE    200
#define SPD_SLIGHT  75
#define SPD_STRONG  150

#define GYRO_SCALE     0.01527f
#define INTEG_THRESH   1.0f      // 角速度 > 此值才积分 (度/秒)
#define LOST_ANGLE     30.0f     // 脱线最大转向角度 (度)
#define LOST_KP        4.0f      // 脱线转向比例系数
#define LOST_PWM_MIN   80
#define LOST_PWM_MAX   250

#define LOOP_MS        10

static float gyro_bias;

static void Gyro_Calibrate(void)
{
    int32_t sum = 0;
    uint16_t i;
    for (i = 0; i < 500; i++) {
        sum += MPU6050_ReadGyroZ();
        delay_us(2500);
    }
    gyro_bias = sum / 500.0f;
}

static int16_t clamp_pwm(int16_t v)
{
    if (v > LOST_PWM_MAX) return LOST_PWM_MAX;
    if (v < -LOST_PWM_MAX) return -LOST_PWM_MAX;
    if (v > 0 && v < LOST_PWM_MIN) return LOST_PWM_MIN;
    if (v < 0 && v > -LOST_PWM_MIN) return -LOST_PWM_MIN;
    return v;
}

int main(void)
{
    float yaw = 0.0f;
    float yaw_rate;
    float lost_yaw = 0.0f;     // 脱线时的 yaw 快照
    float target_offset = 0.0f; // 目标偏移角 (正=左转, 负=右转)
    float error;
    int16_t pwm_l, pwm_r, pwm;
    int16_t gz_raw;
    uint8_t gs;
    uint8_t SL, LM, M, RM, SR;
    uint8_t lost = 0;           // 脱线标志
    uint32_t tick_now, tick_prev, dt_us;

    DWT_Init();
    Motor_Init();
    Grayscale_Init();
    MPU6050_Init();

    Gyro_Calibrate();
    tick_prev = DWT_CYCCNT;

    while (1) {

        /* 时间差 */
        tick_now = DWT_CYCCNT;
        dt_us = (tick_now - tick_prev) / (SystemCoreClock / 1000000);
        tick_prev = tick_now;

        /* 陀螺仪积分 */
        gz_raw = MPU6050_ReadGyroZ();
        yaw_rate = (gz_raw - gyro_bias) * GYRO_SCALE;

        if (yaw_rate > INTEG_THRESH || yaw_rate < -INTEG_THRESH) {
            yaw += yaw_rate * ((float)dt_us / 1000000.0f);
        }

        /* 灰度传感器 */
        gs = Grayscale_Read();
        SL = (gs >> 4) & 1;
        LM = (gs >> 3) & 1;
        M  = (gs >> 2) & 1;
        RM = (gs >> 1) & 1;
        SR = (gs >> 0) & 1;

        if (SL || LM || M || RM || SR) {
            /* === 检测到线：正常循迹 === */
            lost = 0;

            if (M == 1) {
                pwm_l = SPD_BASE;
                pwm_r = SPD_BASE;
            } else if (LM == 1) {
                pwm_l = SPD_BASE - SPD_SLIGHT;
                pwm_r = SPD_BASE + SPD_SLIGHT;
            } else if (RM == 1) {
                pwm_l = SPD_BASE + SPD_SLIGHT;
                pwm_r = SPD_BASE - SPD_SLIGHT;
            } else if (SL == 1) {
                pwm_l = SPD_BASE - SPD_STRONG;
                pwm_r = SPD_BASE + SPD_STRONG;
            } else {  // SR == 1
                pwm_l = SPD_BASE + SPD_STRONG;
                pwm_r = SPD_BASE - SPD_STRONG;
            }

            Motor_SetSpeed(-pwm_l, -pwm_r);

        } else {
            /* === 脱线：MPU6050 辅助恢复 === */
            if (!lost) {
                /* 刚脱线：记录 yaw，根据上次传感器方向设定偏移角 */
                lost = 1;
                lost_yaw = yaw;

                if (SL || LM) {
                    /* 线在左边 → 左转找线 */
                    target_offset = LOST_ANGLE;
                } else if (SR || RM) {
                    /* 线在右边 → 右转找线 */
                    target_offset = -LOST_ANGLE;
                } else {
                    /* 直行丢失 → 保持直行 */
                    target_offset = 0.0f;
                }
            }

            /* 计算当前相对偏移角 */
            float yaw_change = yaw - lost_yaw;
            float err = target_offset - yaw_change;

            if (target_offset == 0.0f) {
                /* 直行丢失：两轮同速前进 */
                Motor_SetSpeed(-SPD_BASE, -SPD_BASE);
            } else {
                /* 转向找线：P 控制偏移角 */
                pwm = clamp_pwm((int16_t)(LOST_KP * err));

                if (target_offset > 0) {
                    /* 左转：左慢右快 */
                    pwm_l = SPD_BASE - pwm;
                    pwm_r = SPD_BASE + pwm;
                } else {
                    /* 右转：左快右慢 */
                    pwm_l = SPD_BASE + pwm;
                    pwm_r = SPD_BASE - pwm;
                }

                /* 限幅 */
                if (pwm_l < 0) pwm_l = 0;
                if (pwm_r < 0) pwm_r = 0;

                Motor_SetSpeed(-pwm_l, -pwm_r);
            }
        }

        delay_us(LOOP_MS * 1000);
    }
}
