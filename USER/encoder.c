#include "encoder.h"

void Encoder_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2  | RCC_APB1Periph_TIM4,  ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;

    // 左轮编码器 PA0 PA1（TIM2）
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 右轮编码器 PB6 PB7（TIM4）
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // TIM2 编码器模式（左轮）
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period        = 65535;
    TIM_TimeBaseStructure.TIM_Prescaler     = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12,
                               TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_Cmd(TIM2, ENABLE);

    // TIM4 编码器模式（右轮）
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12,
                               TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_Cmd(TIM4, ENABLE);

    TIM_SetCounter(TIM2, 0);
    TIM_SetCounter(TIM4, 0);
}

int16_t Encoder_Read_Left(void)
{
    int16_t val = (int16_t)TIM_GetCounter(TIM2);
    TIM_SetCounter(TIM2, 0);
    return val;
}

int16_t Encoder_Read_Right(void)
{
    int16_t val = (int16_t)TIM_GetCounter(TIM4);
    TIM_SetCounter(TIM4, 0);
    return val;
}