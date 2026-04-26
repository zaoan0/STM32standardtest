#include "motor.h"

// 左轮方向
#define L_F() do{ GPIO_SetBits(GPIOB,GPIO_Pin_0); GPIO_ResetBits(GPIOB,GPIO_Pin_1); }while(0)
#define L_B() do{ GPIO_ResetBits(GPIOB,GPIO_Pin_0); GPIO_SetBits(GPIOB,GPIO_Pin_1); }while(0)
#define L_S() do{ GPIO_ResetBits(GPIOB,GPIO_Pin_0); GPIO_ResetBits(GPIOB,GPIO_Pin_1); }while(0)

// 右轮方向
#define R_F() do{ GPIO_SetBits(GPIOB,GPIO_Pin_10); GPIO_ResetBits(GPIOB,GPIO_Pin_11); }while(0)
#define R_B() do{ GPIO_ResetBits(GPIOB,GPIO_Pin_10); GPIO_SetBits(GPIOB,GPIO_Pin_11); }while(0)
#define R_S() do{ GPIO_ResetBits(GPIOB,GPIO_Pin_10); GPIO_ResetBits(GPIOB,GPIO_Pin_11); }while(0)

void Motor_Init(void)
{
    // 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // STBY 拉高（PB14）
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, GPIO_Pin_14);

    // IN1/IN2/IN3/IN4 方向引脚（PB0 PB1 PB10 PB11）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // PWM 引脚 PA6 PA7（TIM3_CH1 CH2，复用推挽）
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // TIM3 配置 PWM，1kHz
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period        = 999;   // ARR
    TIM_TimeBaseStructure.TIM_Prescaler     = 71;    // PSC，72MHz/72=1MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    // PWM 模式
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse       = 0;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);  // CH1 左轮
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);  // CH2 右轮
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);

    Motor_Stop();
}

void Motor_SetSpeed(int16_t left, int16_t right)
{
    if (left  >  PWM_MAX) left  =  PWM_MAX;
    if (left  < -PWM_MAX) left  = -PWM_MAX;
    if (right >  PWM_MAX) right =  PWM_MAX;
    if (right < -PWM_MAX) right = -PWM_MAX;

    if      (left > 0) { L_F(); }
    else if (left < 0) { L_B(); }
    else               { L_S(); }

    if      (right > 0) { R_F(); }
    else if (right < 0) { R_B(); }
    else                { R_S(); }

    TIM_SetCompare1(TIM3, (uint16_t)abs(left));
    TIM_SetCompare2(TIM3, (uint16_t)abs(right));
}

void Motor_Stop(void)
{
    L_S(); R_S();
    TIM_SetCompare1(TIM3, 0);
    TIM_SetCompare2(TIM3, 0);
}