#ifndef __TOUCH_H
#define __TOUCH_H

#include "stm32f10x.h"

/* 触摸点结构体 */
typedef struct {
    uint8_t  num;    /* 触摸点数量（0-5） */
    uint16_t x[5];   /* X坐标 */
    uint16_t y[5];   /* Y坐标 */
} Touch_Data_t;

/* 公共接口 */
void     Touch_Init(void);
uint8_t  Touch_Scan(Touch_Data_t *data);
uint8_t  Touch_IsTouched(void);

/* I2C引脚：PB1(SCL), PF9(SDA) - 软件I2C */
/* 复位：PF11, 中断：PF10（可选，轮询模式未使用） */

#endif
