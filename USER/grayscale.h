#ifndef __GRAYSCALE_H
#define __GRAYSCALE_H
#include "stm32f10x.h"

void Grayscale_Init(void);
uint8_t Grayscale_Read(void); // 返回5位值，bit0=OUT1...bit4=OUT5

#define GS_OUT1  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3)
#define GS_OUT2  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4)
#define GS_OUT3  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5)
#define GS_OUT4  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)
#define GS_OUT5  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)

#endif