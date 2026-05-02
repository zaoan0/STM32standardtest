#ifndef __TOUCH_H
#define __TOUCH_H

#include "stm32f10x.h"

/* Touch point structure */
typedef struct {
    uint8_t  num;    /* Number of touch points (0-5) */
    uint16_t x[5];   /* X coordinates */
    uint16_t y[5];   /* Y coordinates */
} Touch_Data_t;

/* Public API */
void     Touch_Init(void);
uint8_t  Touch_Scan(Touch_Data_t *data);
uint8_t  Touch_IsTouched(void);

/* I2C pins: PB1(SCL), PF9(SDA) - software I2C */
/* Reset: PF11, Interrupt: PF10 (optional, not used in polling mode) */

#endif
