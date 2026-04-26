#include "debug.h"
#include <stdarg.h>
#include <string.h>

#define USART_PORT USART1
#define USART_BAUD 115200

void Debug_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    
    // PA9: USART1_TX (推挽输出)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // PA10: USART1_RX (浮动输入)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // USART1 配置
    USART_InitStructure.USART_BaudRate = USART_BAUD;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void Debug_SendByte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, byte);
}

void Debug_SendStr(const char *str)
{
    while (*str) {
        Debug_SendByte(*str++);
    }
}

void Debug_Printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    
    char buffer[128];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);
    
    Debug_SendStr(buffer);
}

// 重定向 printf 到 USART1
int fputc(int ch, FILE *f)
{
    Debug_SendByte((uint8_t)ch);
    return ch;
}
