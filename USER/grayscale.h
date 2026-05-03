#ifndef __GRAYSCALE_H
#define __GRAYSCALE_H
#include "stm32f10x.h"

/* 8-channel analog grayscale sensor (感为 GW-8ARS)
 * AD0=PC4, AD1=PC5, AD2=PC1 (address select, push-pull output)
 * OUT=PA4 (ADC1_CH4, analog input)
 * EN=NC (internal pull-down, default enabled)
 */

#define GS_CHANNELS  8

void     Grayscale_Init(void);
uint16_t Grayscale_ReadCh(uint8_t ch);    /* read single channel (0-7), return 0-4095 */
void     Grayscale_ReadAll(uint16_t *buf); /* read all 8 channels */

#endif
