#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate 16x16 Chinese font bitmaps for STM32 LCD project.
Uses GB2312 encoding for ARMCC V5 compatibility."""

from PIL import Image, ImageFont, ImageDraw

FONT_PATH = r'C:\Windows\Fonts\simhei.ttf'
FONT_SIZE = 16

# All unique Chinese characters used in the UI
CHARS = '选择模式循迹小车陀螺仪直线待开发返回停止参数调节基准微强弱力状态运行中已目标角度速始敬请期待校准请保持偏差丢失检测修正成减增'

def char_to_bitmap(ch, font):
    img = Image.new('1', (16, 16), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), ch, font=font, fill=1)
    data = []
    for row in range(16):
        b1, b2 = 0, 0
        for col in range(8):
            if img.getpixel((col, row)):
                b1 |= (0x80 >> col)
        for col in range(8, 16):
            if img.getpixel((col, row)):
                b2 |= (0x80 >> (col - 8))
        data.extend([b1, b2])
    return data

def main():
    font = ImageFont.truetype(FONT_PATH, FONT_SIZE)

    # Generate GB2312 encoded font string
    gb_bytes = CHARS.encode('gb2312')

    lines = []
    lines.append('/* Auto-generated 16x16 Chinese font (SimHei, GB2312 encoded) */')
    lines.append('/* Each entry: 32 bytes = 2 bytes/row x 16 rows, MSB=left */')
    lines.append('/* Source file MUST be saved as GB2312/GBK encoding */')
    lines.append('')
    lines.append('/* Font data indexed by position in cn_font_str */')
    lines.append('static const uint8_t cn_font_data[][32] = {')

    for ch in CHARS:
        bitmap = char_to_bitmap(ch, font)
        hex_str = ','.join('0x%02X' % b for b in bitmap)
        lines.append('    /* %s */ {%s},' % (ch, hex_str))

    lines.append('};')
    lines.append('')

    # Generate GB2312 hex string for the lookup table
    hex_gb = ','.join('0x%02X' % b for b in gb_bytes)
    lines.append('/* GB2312 encoded string (2 bytes per Chinese char) */')
    lines.append('static const uint8_t cn_font_str[] = {%s, 0x00};' % hex_gb)
    lines.append('#define CN_FONT_COUNT  %d' % len(CHARS))

    with open('USER/cn_font.h', 'w', encoding='gb2312') as f:
        f.write('\n'.join(lines) + '\n')

    print('Generated USER/cn_font.h with %d characters (GB2312 encoded)' % len(CHARS))
    print('GB2312 bytes: %d' % len(gb_bytes))

if __name__ == '__main__':
    main()
