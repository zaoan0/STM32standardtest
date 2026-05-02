#include "stm32f10x.h"
#include "delay.h"
#include "debug.h"
#include "lcd.h"
#include "touch.h"
#include "tracking.h"
#include "ui.h"

/*
 * v2.0: 战舰V3 ZET6 + 4.3" TFT LCD 触摸屏循迹小车
 * 模块化重构：tracking.c 负责循迹控制，ui.c 负责界面交互
 */

#define LOOP_MS  10

int main(void)
{
    uint32_t tick_now, tick_prev, dt_us;
    static uint32_t disp_tick = 0;

    /* 初始化基础模块 */
    DWT_Init();
    Debug_Init();
    Debug_Printf("=== BOOT ===\r\n");

    /* 初始化硬件驱动 */
    Tracking_Init();

    /* 初始化 LCD */
    LCD_Init();
    Debug_Printf("LCD OK\r\n");

    /* 初始化触摸屏 */
    Touch_Init();
    Debug_Printf("Touch OK\r\n");

    Debug_Printf("=== ALL INIT DONE ===\r\n");

    /* 绘制 UI */
    UI_Init();

    /* 陀螺仪校准 */
    Tracking_Gyro_Calibrate();
    Debug_Printf("System ready. 72MHz, LCD 800x480\r\n");

    tick_prev = DWT_CYCCNT;

    /* 初始状态显示 */
    UI_UpdateStatus();

    while (1) {
        /* 计算时间差 */
        tick_now = DWT_CYCCNT;
        dt_us = (tick_now - tick_prev) / (SystemCoreClock / 1000000);
        tick_prev = tick_now;

        /* 陀螺仪积分 */
        Tracking_UpdateGyro(dt_us);

        /* 触摸处理 */
        UI_HandleTouch(dt_us);

        /* 循迹控制 */
        Tracking_Run(dt_us);

        /* 状态显示 (每 200ms) */
        disp_tick += dt_us;
        if (disp_tick >= 200000) {
            disp_tick = 0;
            UI_UpdateStatus();
        }

        delay_us(LOOP_MS * 1000);
    }
}
