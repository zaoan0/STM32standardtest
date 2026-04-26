#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"
#include <stdlib.h>

#define PWM_MAX 999

void Motor_Init(void);
void Motor_SetSpeed(int16_t left, int16_t right);
void Motor_Stop(void);

#endif