#include "stm32f10x.h"
#include "motor.h"
#include "grayscale.h"

void delay(uint32_t n){ while(n--); }

#define SPD_BASE    200
#define SPD_SLIGHT  75
#define SPD_STRONG  150


// 脱线模式选择：
// 1 = 脱线后直走（保持上一个速度也行，这里用直走）
// 0 = 脱线后保持上一次转向状态
#define LOST_MODE   0

int main(void){
    Motor_Init();
    Grayscale_Init();

    uint8_t gs;
    uint8_t SL, LM, M, RM, SR;
    int16_t pwm_l = SPD_BASE;
    int16_t pwm_r = SPD_BASE;
    int16_t last_pwm_l = SPD_BASE; // 记录上一次有效状态
    int16_t last_pwm_r = SPD_BASE;

    while(1){
        delay(50000);

        gs = Grayscale_Read();

        SL = (gs >> 4) & 1; // 最左
        LM = (gs >> 3) & 1; // 左中
        M  = (gs >> 2) & 1; // 中间
        RM = (gs >> 1) & 1; // 右中
        SR = (gs >> 0) & 1; // 最右

        if (M == 1) {
            // 中间在黑线，直走
            pwm_l = SPD_BASE;
            pwm_r = SPD_BASE;
        } else if (LM == 1) {
            // 偏左，向右修正
            pwm_l = SPD_BASE - SPD_SLIGHT;
            pwm_r = SPD_BASE + SPD_SLIGHT;
        } else if (RM == 1) {
            // 偏右，向左修正
            pwm_l = SPD_BASE + SPD_SLIGHT;
            pwm_r = SPD_BASE - SPD_SLIGHT;
        } else if (SL == 1) {
            // 大幅偏左
            pwm_l = SPD_BASE - SPD_STRONG;
            pwm_r = SPD_BASE + SPD_STRONG;
        } else if (SR == 1) {
            // 大幅偏右
            pwm_l = SPD_BASE + SPD_STRONG;
            pwm_r = SPD_BASE - SPD_STRONG;
        } else {
            // 脱线处理
#if LOST_MODE == 1
            // 模式1：直走等待重新找到线
            pwm_l = SPD_BASE;
            pwm_r = SPD_BASE;
#else
            // 模式0：保持上一次转向状态继续寻线
            pwm_l = last_pwm_l;
            pwm_r = last_pwm_r;
#endif
            Motor_SetSpeed(-pwm_l, -pwm_r);
            continue;
        }

        // 保存本次有效状态
        last_pwm_l = pwm_l;
        last_pwm_r = pwm_r;

        Motor_SetSpeed(-pwm_l, -pwm_r);
    }
}