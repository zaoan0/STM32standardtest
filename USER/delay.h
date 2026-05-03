#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

// DWT寄存器地址（StdPeriph V3.5.0 CMSIS未定义）
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)
#define DWT_CTRL_CYCCNTENA  (1UL << 0)

void delay_init(void);
void delay_ms(uint32_t ms);
void DWT_Init(void);
void delay_us(uint32_t us);

#endif
