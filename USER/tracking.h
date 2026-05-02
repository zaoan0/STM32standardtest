#ifndef __TRACKING_H
#define __TRACKING_H

#include "stm32f10x.h"

/* ---- 模式定义 ---- */
#define MODE_STOP      0
#define MODE_TRACK     1

extern uint8_t current_mode;

/* ---- 初始化与校准 ---- */
void    Tracking_Init(void);
void    Tracking_Gyro_Calibrate(void);  /* 调用前请在LCD上显示提示文字 */

/* ---- 主循环调用 ---- */
void    Tracking_UpdateGyro(uint32_t dt_us);  /* 陀螺仪积分，每循环调用 */
void    Tracking_Run(uint32_t dt_us);         /* 循迹/脱线恢复控制 */

/* ---- 状态查询（供UI显示） ---- */
float   Tracking_GetYaw(void);
uint8_t Tracking_GetGS(void);
uint8_t Tracking_GetLost(void);

/* ---- 参数读写（供UI调参） ---- */
void    Tracking_SetBaseSpeed(int16_t v);
void    Tracking_SetSlightSpeed(int16_t v);
void    Tracking_SetStrongSpeed(int16_t v);
int16_t Tracking_GetBaseSpeed(void);
int16_t Tracking_GetSlightSpeed(void);
int16_t Tracking_GetStrongSpeed(void);

#endif
