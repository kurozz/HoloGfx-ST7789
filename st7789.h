#ifndef __ST7789_H
#define __ST7789_H

#include "fonts/fonts.h"
#include "main.h"

/* Display type enumeration */
typedef enum {
	ST7789_DISPLAY_135x240 = 0,
	ST7789_DISPLAY_240x240 = 1,
	ST7789_DISPLAY_170x320 = 2
} ST7789_DisplayType_t;

/* Display configuration structure */
typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t x_shift;
	uint16_t y_shift;
	uint8_t rotation;
	ST7789_DisplayType_t display_type;
} ST7789_Config_t;

/* choose a Hardware SPI port to use. */
#define ST7789_SPI_PORT hspi1
extern SPI_HandleTypeDef ST7789_SPI_PORT;

/* choose whether use DMA or not */
#define ST7789_USE_DMA

/* Pin connection*/
#define ST7789_RST_PORT ST7789_RST_GPIO_Port
#define ST7789_RST_PIN  ST7789_RST_Pin
#define ST7789_DC_PORT  ST7789_DC_GPIO_Port
#define ST7789_DC_PIN   ST7789_DC_Pin
#define ST7789_CS_PORT  ST7789_CS_GPIO_Port
#define ST7789_CS_PIN   ST7789_CS_Pin

/* Runtime configuration (preferred over compile-time defines) */
extern ST7789_Config_t st7789_config;

/**
 *Color of pen
 *If you want to use another color, you can choose one in RGB565 format.
 */

#define ST7789_COLOR_WHITE       0xFFFF
#define ST7789_COLOR_BLACK       0x0000
#define ST7789_COLOR_BLUE        0x001F
#define ST7789_COLOR_RED         0xF800
#define ST7789_COLOR_MAGENTA     0xF81F
#define ST7789_COLOR_GREEN       0x07E0
#define ST7789_COLOR_CYAN        0x7FFF
#define ST7789_COLOR_YELLOW      0xFFE0
#define ST7789_COLOR_GRAY        0x8430
#define ST7789_COLOR_BRED        0xF81F
#define ST7789_COLOR_GRED        0xFFE0
#define ST7789_COLOR_GBLUE       0x07FF
#define ST7789_COLOR_BROWN       0xBC40
#define ST7789_COLOR_BRRED       0xFC07
#define ST7789_COLOR_DARKBLUE    0x01CF
#define ST7789_COLOR_LIGHTBLUE   0x7D7C
#define ST7789_COLOR_GRAYBLUE    0x5458

#define ST7789_COLOR_LIGHTGREEN  0x841F
#define ST7789_COLOR_LGRAY       0xC618
#define ST7789_COLOR_LGRAYBLUE   0xA651
#define ST7789_COLOR_LBBLUE      0x2B12

/* Runtime accessors for display parameters (use these instead of compile-time defines) */
#define ST7789_WIDTH   (st7789_config.width)
#define ST7789_HEIGHT  (st7789_config.height)
#define ST7789_X_SHIFT (st7789_config.x_shift)
#define ST7789_Y_SHIFT (st7789_config.y_shift)

/* Basic functions. */
void ST7789_Init(ST7789_DisplayType_t display_type, uint8_t rotation, uint16_t buffer_size_bytes);
void ST7789_Deinit(void);
void ST7789_SetRotation(uint8_t rotation);
void ST7789_Fill_Color(uint16_t color);
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color);

/* Graphical functions. */
void ST7789_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void ST7789_InvertColors(uint8_t invert);

/* Text functions. */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, const GFXfont *font, uint16_t color, uint16_t bgcolor);
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, const GFXfont *font, uint16_t color, uint16_t bgcolor);

/* Extended Graphical functions. */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color);
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/* Command functions */
void ST7789_TearEffect(uint8_t tear);

/* Simple test function. */
void ST7789_Test(void);

#endif
