#ifndef __DEBUG_H
#define __DEBUG_H

#include "stm32f10x.h"
#include <stdio.h>

void Debug_Init(void);
void Debug_Printf(const char *format, ...);

#endif
