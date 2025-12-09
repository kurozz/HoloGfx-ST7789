#ifndef __ST7789_COLORS_H
#define __ST7789_COLORS_H

/**
 * @file st7789_colors.h
 * @brief RGB565 color definitions for ST7789 display driver
 *
 * Color format: RGB565 (16-bit)
 * - Red:   5 bits (bits 15-11)
 * - Green: 6 bits (bits 10-5)
 * - Blue:  5 bits (bits 4-0)
 */

/* Primary Colors */
#define ST7789_COLOR_WHITE       0xFFFF
#define ST7789_COLOR_BLACK       0x0000
#define ST7789_COLOR_RED         0xF800
#define ST7789_COLOR_GREEN       0x07E0
#define ST7789_COLOR_BLUE        0x001F

/* Secondary Colors */
#define ST7789_COLOR_YELLOW      0xFFE0
#define ST7789_COLOR_CYAN        0x7FFF
#define ST7789_COLOR_MAGENTA     0xF81F

/* Grayscale */
#define ST7789_COLOR_GRAY        0x8430
#define ST7789_COLOR_LGRAY       0xC618

/* Extended Palette */
#define ST7789_COLOR_BRED        0xF81F
#define ST7789_COLOR_GRED        0xFFE0
#define ST7789_COLOR_GBLUE       0x07FF
#define ST7789_COLOR_BROWN       0xBC40
#define ST7789_COLOR_BRRED       0xFC07
#define ST7789_COLOR_DARKBLUE    0x01CF
#define ST7789_COLOR_LIGHTBLUE   0x7D7C
#define ST7789_COLOR_GRAYBLUE    0x5458
#define ST7789_COLOR_LIGHTGREEN  0x841F
#define ST7789_COLOR_LGRAYBLUE   0xA651
#define ST7789_COLOR_LBBLUE      0x2B12

#endif /* __ST7789_COLORS_H */
