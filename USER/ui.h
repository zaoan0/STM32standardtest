#ifndef __UI_H
#define __UI_H

#include "stm32f10x.h"

/* ---- 页面定义 ---- */
typedef enum {
    PAGE_MAIN = 0,
    PAGE_TRACK,
    PAGE_GYRO,
    PAGE_BLANK1,
    PAGE_BLANK2,
    PAGE_NUM
} Page_t;

void    UI_Init(void);                   /* 绘制当前页面 */
void    UI_HandleTouch(uint32_t dt_us);  /* 触摸扫描+事件处理，每循环调用 */
void    UI_UpdateStatus(void);           /* 刷新当前页面的状态显示 */

Page_t  UI_GetPage(void);                /* 获取当前页面 */
void    UI_NavigateTo(Page_t page);      /* 跳转到指定页面 */

#endif
