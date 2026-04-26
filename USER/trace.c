#include "trace.h"

void Trace_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 |
                                   GPIO_Pin_12 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

static uint8_t s(uint16_t pin)
{
    return !GPIO_ReadInputDataBit(GPIOB, pin);  // 低电平有效
}

int8_t Trace_GetError(void)
{
    uint8_t s1=s(GPIO_Pin_3), s2=s(GPIO_Pin_4), s3=s(GPIO_Pin_5),
            s4=s(GPIO_Pin_12), s5=s(GPIO_Pin_13);
    uint8_t total = s1+s2+s3+s4+s5;
    if (total == 0) return 0;
    return (int8_t)(s1*(-2) + s2*(-1) + s3*0 + s4*1 + s5*2);
}

uint8_t Trace_AllBlack(void)
{
    return s(GPIO_Pin_3) && s(GPIO_Pin_4) && s(GPIO_Pin_5)
        && s(GPIO_Pin_12) && s(GPIO_Pin_13);
}