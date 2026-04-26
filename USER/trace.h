#ifndef __TRACE_H
#define __TRACE_H

#include "stm32f10x.h"
#include <stdint.h>

void   Trace_Init(void);
int8_t Trace_GetError(void);
uint8_t Trace_AllBlack(void);

#endif