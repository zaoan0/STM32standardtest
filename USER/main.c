#include "stm32f10x.h"
#include "motor.h"
#include "grayscale.h"

void delay(uint32_t n){ while(n--); }

#define SPD_BASE      200
#define SPD_SLIGHT     75
#define SPD_STRONG    150
#define STABLE_COUNT   15   // 约0.5s，可调

int main(void){
    Motor_Init();
    Grayscale_Init();

    uint8_t gs;
    uint8_t SL, LM, M, RM, SR;
    int16_t pwm_l = SPD_BASE;
    int16_t pwm_r = SPD_BASE;
    int16_t last_pwm_l = SPD_BASE;
    int16_t last_pwm_r = SPD_BASE;

    uint8_t  lost_flag  = 0;
    uint16_t stable_cnt = 0;

    while(1){
        delay(50000);

        gs = Grayscale_Read();
        SL = (gs >> 4) & 1;
        LM = (gs >> 3) & 1;
        M  = (gs >> 2) & 1;
        RM = (gs >> 1) & 1;
        SR = (gs >> 0) & 1;

        if(lost_flag){
            Motor_SetSpeed(-last_pwm_l, -last_pwm_r);
            if((SL + LM + M + RM + SR) == 1){
                if(++stable_cnt >= STABLE_COUNT){
                    lost_flag  = 0;
                    stable_cnt = 0;
                }
            } else {
                stable_cnt = 0;
            }
            continue;
        }

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
