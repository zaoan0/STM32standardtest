#include "grayscale.h"

void Grayscale_Init(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    
    // 禁用JTAG，释放PB3 PB4（保留SWD供ST-Link用）
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5
                                  |GPIO_Pin_12|GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

uint8_t Grayscale_Read(void){
    uint8_t val = 0;
    val |= (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)) << 4; // OUT5最左(车头视角)
    val |= (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)) << 3; // OUT4左中
    val |= (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5))  << 2; // OUT3中间
    val |= (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4))  << 1; // OUT2右中
    val |= (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3))  << 0; // OUT1最右
    return val;
}