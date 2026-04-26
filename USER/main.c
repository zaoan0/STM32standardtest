#include "stm32f10x.h"
#include "motor.h"
#include "grayscale.h"

void delay(uint32_t n){ while(n--); }

#define SPD_BASE    200
#define SPD_SLIGHT  75
#define SPD_STRONG  150
#define STABLE_COUNT 15   // 约0.5s，可调

int main(void){
    Motor_Init();
    Grayscale_Init();

    uint8_t gs;
    uint8_t SL, LM, M, RM, SR;
    int16_t pwm_l = SPD_BASE;
    int16_t pwm_r = SPD_BASE;
    int16_t last_pwm_l = SPD_BASE;
    int16_t last_pwm_r = SPD_BASE;

    uint8_t  lost_flag  = 0;  // 脱线标志
    uint16_t stable_cnt = 0;  // 稳定计数

    while(1){
        delay(50000);

        gs = Grayscale_Read();
        SL = (gs >> 4) & 1;
        LM = (gs >> 3) & 1;
        M  = (gs >> 2) & 1;
        RM = (gs >> 1) & 1;
        SR = (gs >> 0) & 1;

        // ===== 脱线处理：保持上次状态，等只有一个传感器稳定0.5s =====
        if(lost_flag){
            Motor_SetSpeed(-last_pwm_l, -last_pwm_r);
            if((SL + LM + M + RM + SR) == 1){  // 只有一个传感器检测到线，认为是稳定状态
                if(++stable_cnt >= STABLE_COUNT){
                    lost_flag  = 0;  // 恢复正常循迹
                    stable_cnt = 0;
                }
            } else {
                stable_cnt = 0;  // 条件被打断，重新计数
            }
            continue;
        }

        // ===== 以下是你的原代码，一字未改 =====
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
        } else if (SR == 1) {
            pwm_l = SPD_BASE + SPD_STRONG;
            pwm_r = SPD_BASE - SPD_STRONG;
        } else {
            // 全脱线，进入脱线模式
            lost_flag  = 1;
            stable_cnt = 0;
            Motor_SetSpeed(-last_pwm_l, -last_pwm_r);
            continue;
        }

        last_pwm_l = pwm_l;
        last_pwm_r = pwm_r;
        Motor_SetSpeed(-pwm_l, -pwm_r);
    }
}