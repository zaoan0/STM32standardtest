#ifndef __UI_H
#define __UI_H

#include "stm32f10x.h"

void UI_Init(void);                   /* 绘制完整UI界面 */
void UI_HandleTouch(uint32_t dt_us);  /* 触摸扫描+事件处理，每循环调用 */
void UI_UpdateStatus(void);           /* 刷新状态显示区域 */

#endif
