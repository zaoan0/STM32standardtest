#include "delay.h"

static uint32_t fac_ms;

void delay_init(void)
{
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
    fac_ms = SystemCoreClock / 8000;
}

void delay_ms(uint32_t ms)
{
    uint32_t temp;
    SysTick->LOAD = ms * fac_ms;
    SysTick->VAL  = 0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    do {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1<<16)));
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL = 0x00;
}

void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT_CYCCNT = 0;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;
}

void delay_us(uint32_t us)
{
    uint32_t start = DWT_CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT_CYCCNT - start) < ticks);
}