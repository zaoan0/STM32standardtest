#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"

void    Encoder_Init(void);
int16_t Encoder_Read_Left(void);
int16_t Encoder_Read_Right(void);

#endif