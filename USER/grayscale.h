#ifndef __GRAYSCALE_H
#define __GRAYSCALE_H
#include "stm32f10x.h"

/* 8路模拟 Grayscale Sensor（感为 GW-8ARS）
 * AD0=PC4, AD1=PC5, AD2=PC1（地址选择，Push-pull output）
 * OUT=PA4（ADC1_CH4，Analog input）
 * EN=NC（内部 Pull-down，默认 Enable）
 */

#define GS_CHANNELS  8

void     Grayscale_Init(void);
uint16_t Grayscale_ReadCh(uint8_t ch);    /* 读取单通道（0-7），返回0-4095 */
void     Grayscale_ReadAll(uint16_t *buf); /* 读取全部8路通道 */

#endif
