#include "grayscale.h"
#include "delay.h"

/* 8-channel analog grayscale sensor (感为 GW-8ARS)
 * AD0=PC4, AD1=PC5, AD2=PC1 (address select, push-pull output)
 * OUT=PA4 (ADC1_CH4, analog input)
 * EN=NC (internal pull-down, default enabled)
 */

#define AD0_H  GPIO_SetBits(GPIOC, GPIO_Pin_4)
#define AD0_L  GPIO_ResetBits(GPIOC, GPIO_Pin_4)
#define AD1_H  GPIO_SetBits(GPIOC, GPIO_Pin_5)
#define AD1_L  GPIO_ResetBits(GPIOC, GPIO_Pin_5)
#define AD2_H  GPIO_SetBits(GPIOC, GPIO_Pin_1)
#define AD2_L  GPIO_ResetBits(GPIOC, GPIO_Pin_1)

static void GS_SelectCh(uint8_t ch)
{
    if (ch & 0x01) AD0_H; else AD0_L;
    if (ch & 0x02) AD1_H; else AD1_L;
    if (ch & 0x04) AD2_H; else AD2_L;
    delay_us(5);
}

void Grayscale_Init(void)
{
    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC |
                           RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);  /* ADC clock = 12MHz */

    /* PC1, PC4, PC5: push-pull output (address select) */
    gpio.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &gpio);

    /* PA4: analog input (ADC) */
    gpio.GPIO_Pin   = GPIO_Pin_4;
    gpio.GPIO_Mode  = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);

    /* ADC1 config */
    adc.ADC_Mode               = ADC_Mode_Independent;
    adc.ADC_ScanConvMode       = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign          = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel       = 1;
    ADC_Init(ADC1, &adc);

    ADC_Cmd(ADC1, ENABLE);

    /* ADC calibration */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

uint16_t Grayscale_ReadCh(uint8_t ch)
{
    GS_SelectCh(ch);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_239Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}

void Grayscale_ReadAll(uint16_t *buf)
{
    uint8_t i;
    for (i = 0; i < GS_CHANNELS; i++) {
        buf[i] = Grayscale_ReadCh(i);
    }
}
