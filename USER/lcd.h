#ifndef __LCD_H
#define __LCD_H

#include "stm32f10x.h"

/* LCD尺寸（NT35510：800x480 横屏） */
#define LCD_W   800
#define LCD_H   480

/* 16位颜色（RGB565） */
#define WHITE       0xFFFF
#define BLACK       0x0000
#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define YELLOW      0xFFE0
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define GRAY        0x8410
#define DARKGRAY    0x4208
#define LIGHTGRAY   0xC618
#define ORANGE      0xFD20

/* 公共接口 */
void LCD_Init(void);
uint16_t LCD_ReadID(void);
void LCD_Clear(uint16_t color);
void LCD_SetCursor(uint16_t x, uint16_t y);
void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg, uint8_t size);
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);
void LCD_ShowNum(uint16_t x, uint16_t y, int32_t num, uint8_t len, uint16_t color, uint16_t bg, uint8_t size);
void LCD_DrawButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char *text, uint16_t fg, uint16_t bg);

/* 中文字符支持（16x16，黑体） */
void LCD_ShowChinese(uint16_t x, uint16_t y, const char *utf8_ch, uint16_t color, uint16_t bg);
void LCD_ShowMixedString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);
void LCD_DrawButtonCN(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char *text, uint16_t fg, uint16_t bg);

/* 底层接口（触摸校准用） */
void LCD_WR_REG(uint16_t reg);
void LCD_WR_DATA(uint16_t data);
uint16_t LCD_RD_DATA(void);

#endif
