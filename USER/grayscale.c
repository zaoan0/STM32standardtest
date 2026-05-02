#include "grayscale.h"

/*
 * 战舰V3 引脚重映射（避开板载外设冲突）：
 *   OUT1(最右) PC4   OUT2(右中) PC5   OUT3(中间) PC6
 *   OUT4(左中) PC7   OUT5(最左) PD2
 */

void Grayscale_Init(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    // PC4 PC5 PC6 PC7
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // PD2
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_2;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

uint8_t Grayscale_Read(void){
    uint8_t val = 0;
    val |= (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2)) << 4; // OUT5最左(车头视角)
    val |= (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7)) << 3; // OUT4左中
    val |= (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6)) << 2; // OUT3中间
    val |= (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5)) << 1; // OUT2右中
#if !SR_BROKEN
    val |= (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4)) << 0; // OUT1最右
#endif
    return val;
}