#include "stm32f10x.h"
#include "delay.h"
#include "debug.h"
#include "lcd.h"
#include "touch.h"
#include "tracking.h"
#include "ui.h"
#include "grayscale.h"

/*
 * v2.0：战舰V3 ZET6 + 4.3" TFT LCD Touchscreen 循迹小车
 * 多页面UI：主页导航 → 循迹 / Gyro 直线 / 空白页
 */

#define LOOP_MS  10

int main(void)
{
    uint32_t tick_now, tick_prev, dt_us;
    static uint32_t disp_tick = 0;

    /* 初始化基础外设 */
    DWT_Init();
    Debug_Init();
    Debug_Printf("=== BOOT ===\r\n");

    /* 初始化硬件模块 */
    Tracking_Init();

    /* 初始化LCD */
    LCD_Init();
    Debug_Printf("LCD OK\r\n");

    /* 初始化 Touchscreen */
    Touch_Init();
    Debug_Printf("Touch OK\r\n");

    Debug_Printf("=== ALL INIT DONE ===\r\n");

    /* ADC自检：读取PA4原始值 */
    {
        uint16_t test_adc;
        /* 选择 Multiplexer 通道0 (AD0=0, AD1=0, AD2=0) */
        GPIO_ResetBits(GPIOC, GPIO_Pin_4);  /* AD0 */
        GPIO_ResetBits(GPIOC, GPIO_Pin_5);  /* AD1 */
        GPIO_ResetBits(GPIOC, GPIO_Pin_1);  /* AD2 */
        delay_us(100);
        ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_239Cycles5);
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
        while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
        test_adc = ADC_GetConversionValue(ADC1);
        Debug_Printf("ADC test CH0 = %d\r\n", test_adc);

        /* 选择 Multiplexer 通道7 (AD0=1, AD1=1, AD2=1) */
        GPIO_SetBits(GPIOC, GPIO_Pin_4);  /* AD0 */
        GPIO_SetBits(GPIOC, GPIO_Pin_5);  /* AD1 */
        GPIO_SetBits(GPIOC, GPIO_Pin_1);  /* AD2 */
        delay_us(100);
        ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_239Cycles5);
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
        while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
        test_adc = ADC_GetConversionValue(ADC1);
        Debug_Printf("ADC test CH7 = %d\r\n", test_adc);
    }

    /* Gyro 校准 */
    Tracking_Gyro_Calibrate();
    Debug_Printf("System ready. 72MHz, LCD 800x480\r\n");

    /* 绘制UI（校准后，避免被校准文字覆盖） */
    UI_Init();

    tick_prev = DWT_CYCCNT;

    while (1) {
        /* 计算时间差 */
        tick_now = DWT_CYCCNT;
        dt_us = (tick_now - tick_prev) / (SystemCoreClock / 1000000);
        tick_prev = tick_now;

        /* Gyro 积分（始终运行） */
        Tracking_UpdateGyro(dt_us);

        /* 触摸处理 */
        UI_HandleTouch(dt_us);

        /* 根据当前页面分发控制逻辑 */
        switch (UI_GetPage()) {
            case PAGE_TRACK:
                Tracking_Run(dt_us);
                break;
            case PAGE_GYRO:
                Tracking_GyroStraight_Run(dt_us);
                break;
            default:
                break;
        }

        /* 状态显示（每200ms） */
        disp_tick += dt_us;
        if (disp_tick >= 200000) {
            disp_tick = 0;
            UI_UpdateStatus();
        }

        /* 调试：每500ms打印ADC值（直接读取，任何页面） */
        {
            static uint32_t dbg_tick = 0;
            static uint8_t dbg_first = 1;
            dbg_tick += dt_us;
            if (dbg_tick >= 500000) {
                uint16_t dbg_buf[8];
                dbg_tick = 0;
                if (dbg_first) {
                    Debug_Printf("=== ADC DEBUG START ===\r\n");
                    dbg_first = 0;
                }
                Grayscale_ReadAll(dbg_buf);
                Debug_Printf("GS: %d %d %d %d %d %d %d %d\r\n",
                    dbg_buf[0], dbg_buf[1], dbg_buf[2], dbg_buf[3],
                    dbg_buf[4], dbg_buf[5], dbg_buf[6], dbg_buf[7]);
            }
        }

        delay_us(LOOP_MS * 1000);
    }
}
