#include "st7789.h"
#include <string.h>
#include <stdlib.h>

/* Advanced options */
#define ST7789_COLOR_MODE_16bit 0x55    //  RGB565 (16bit)
#define ST7789_COLOR_MODE_18bit 0x66    //  RGB666 (18bit)

/* Control Registers and constant codes */
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09

#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_PTLON   0x12
#define ST7789_NORON   0x13

#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_PTLAR   0x30
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36

#define ST7789_PORCTRL    0xB2  // Porch Control
#define ST7789_GCTRL      0xB7  // Gate Control
#define ST7789_VCOMS      0xBB  // VCOM Setting
#define ST7789_LCMCTRL    0xC0  // LCM Control
#define ST7789_VDVVRHEN   0xC2  // VDV and VRH Command Enable
#define ST7789_VRHS       0xC3  // VRH Set
#define ST7789_VDVS       0xC4  // VDV Set
#define ST7789_FRCTRL2    0xC6  // Frame Rate Control in Normal Mode
#define ST7789_PWCTRL1    0xD0  // Power Control 1
#define ST7789_PVGAMCTRL  0xE0  // Positive Voltage Gamma Control
#define ST7789_NVGAMCTRL  0xE1  // Negative Voltage Gamma Control

#define ST7789_RDID1   0xDA
#define ST7789_RDID2   0xDB
#define ST7789_RDID3   0xDC
#define ST7789_RDID4   0xDD

/**
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0
 * param:   MY  MX  MV  ML  RGB MH  -   -
 *
 */

/* Page Address Order ('0': Top to Bottom, '1': the opposite) */
#define ST7789_MADCTL_MY  0x80
/* Column Address Order ('0': Left to Right, '1': the opposite) */
#define ST7789_MADCTL_MX  0x40
/* Page/Column Order ('0' = Normal Mode, '1' = Reverse Mode) */
#define ST7789_MADCTL_MV  0x20
/* Line Address Order ('0' = LCD Refresh Top to Bottom, '1' = the opposite) */
#define ST7789_MADCTL_ML  0x10
/* RGB/BGR Order ('0' = RGB, '1' = BGR) */
#define ST7789_MADCTL_RGB 0x00

/* Basic operations */
#define ST7789_RST_Clr() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_RESET)
#define ST7789_RST_Set() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_SET)

#define ST7789_DC_Clr() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_RESET)
#define ST7789_DC_Set() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_SET)
#define ST7789_Select() HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_RESET)
#define ST7789_UnSelect() HAL_GPIO_WritePin(ST7789_CS_PORT, ST7789_CS_PIN, GPIO_PIN_SET)

#define ST7789_ABS(x) ((x) > 0 ? (x) : -(x))

/* Internal constants (moved from header to avoid namespace pollution) */
#define HOR_LEN 5  // Number of horizontal lines to buffer
#define MAX_DISPLAY_WIDTH 320  // Maximum width among all supported displays
#define MIN_BUFFER_SIZE 256  // Minimum buffer size in bytes (128 pixels)

/* Global runtime configuration */
ST7789_Config_t st7789_config = {
	.width = 240,
	.height = 240,
	.x_shift = 0,
	.y_shift = 80,
	.rotation = 0,
	.display_type = ST7789_DISPLAY_240x240
};

/* Display buffer for optimized drawing operations.
 * Dynamically allocated based on display size to optimize RAM usage.
 * Used for both DMA and non-DMA modes to improve performance.
 */
uint16_t *st7789_disp_buf = NULL;
uint16_t st7789_disp_buf_size = 0;  // Size in uint16_t elements (pixels)

#ifdef ST7789_USE_DMA
uint16_t st7789_dma_min_size = 16;
#endif

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void ST7789_WriteCommand(uint8_t cmd)
{
	ST7789_Select();
	ST7789_DC_Clr();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
	ST7789_UnSelect();
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void ST7789_WriteData(uint8_t *buff, size_t buff_size)
{
	ST7789_Select();
	ST7789_DC_Set();

	// split data in small chunks because HAL can't send more than 64K at once

	while (buff_size > 0) {
		uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
		#ifdef ST7789_USE_DMA
			if (st7789_dma_min_size <= buff_size)
			{
				HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, buff, chunk_size);
				while (ST7789_SPI_PORT.hdmatx->State != HAL_DMA_STATE_READY)
				{}
			}
			else
				HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
		#else
			HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
		#endif
		buff += chunk_size;
		buff_size -= chunk_size;
	}

	ST7789_UnSelect();
}
/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void ST7789_WriteSmallData(uint8_t data)
{
	ST7789_Select();
	ST7789_DC_Set();
	HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, sizeof(data), HAL_MAX_DELAY);
	ST7789_UnSelect();
}

/**
 * @brief Allocate or reallocate display buffer
 * @param buffer_size_bytes -> requested buffer size in bytes (0 = auto-calculate optimal)
 * @return 0 on success, -1 on failure
 */
static int ST7789_AllocateBuffer(uint16_t buffer_size_bytes)
{
	uint16_t actual_buffer_size;

	// Calculate optimal buffer size if auto (0)
	if (buffer_size_bytes == 0) {
		// Optimal: display_width * HOR_LEN * 2 bytes
		actual_buffer_size = st7789_config.width * HOR_LEN * 2;
	} else {
		// Use requested size with bounds checking
		uint16_t min_size = MIN_BUFFER_SIZE;
		uint16_t max_size = st7789_config.width * HOR_LEN * 2;

		if (buffer_size_bytes < min_size) {
			actual_buffer_size = min_size;
		} else if (buffer_size_bytes > max_size) {
			actual_buffer_size = max_size;
		} else {
			actual_buffer_size = buffer_size_bytes;
		}
	}

	// Free existing buffer if any
	if (st7789_disp_buf != NULL) {
		free(st7789_disp_buf);
		st7789_disp_buf = NULL;
		st7789_disp_buf_size = 0;
	}

	// Allocate new buffer
	st7789_disp_buf = (uint16_t*)malloc(actual_buffer_size);
	if (st7789_disp_buf == NULL) {
		return -1;  // Allocation failed
	}

	st7789_disp_buf_size = actual_buffer_size / sizeof(uint16_t);
	memset(st7789_disp_buf, 0, actual_buffer_size);

	return 0;  // Success
}

/**
 * @brief Calculate display parameters based on display type and rotation
 * @param type -> display type
 * @param rotation -> rotation value (0-3)
 * @param width -> pointer to store calculated width
 * @param height -> pointer to store calculated height
 * @param x_shift -> pointer to store calculated x shift
 * @param y_shift -> pointer to store calculated y shift
 * @return none
 */
static void ST7789_CalculateDisplayParams(ST7789_DisplayType_t type, uint8_t rotation,
                                           uint16_t *width, uint16_t *height,
                                           uint16_t *x_shift, uint16_t *y_shift)
{
	switch (type) {
		case ST7789_DISPLAY_135x240:
			switch (rotation) {
				case 0:
					*width = 135; *height = 240; *x_shift = 53; *y_shift = 40;
					break;
				case 1:
					*width = 240; *height = 135; *x_shift = 40; *y_shift = 52;
					break;
				case 2:
					*width = 135; *height = 240; *x_shift = 52; *y_shift = 40;
					break;
				case 3:
					*width = 240; *height = 135; *x_shift = 40; *y_shift = 53;
					break;
				default:
					*width = 135; *height = 240; *x_shift = 53; *y_shift = 40;
					break;
			}
			break;

		case ST7789_DISPLAY_240x240:
			switch (rotation) {
				case 0:
					*width = 240; *height = 240; *x_shift = 0; *y_shift = 80;
					break;
				case 1:
					*width = 240; *height = 240; *x_shift = 80; *y_shift = 0;
					break;
				case 2:
					*width = 240; *height = 240; *x_shift = 0; *y_shift = 0;
					break;
				case 3:
					*width = 240; *height = 240; *x_shift = 0; *y_shift = 0;
					break;
				default:
					*width = 240; *height = 240; *x_shift = 0; *y_shift = 80;
					break;
			}
			break;

		case ST7789_DISPLAY_170x320:
			switch (rotation) {
				case 0:
					*width = 170; *height = 320; *x_shift = 35; *y_shift = 0;
					break;
				case 1:
					*width = 320; *height = 170; *x_shift = 0; *y_shift = 35;
					break;
				case 2:
					*width = 170; *height = 320; *x_shift = 35; *y_shift = 0;
					break;
				case 3:
					*width = 320; *height = 170; *x_shift = 0; *y_shift = 35;
					break;
				default:
					*width = 170; *height = 320; *x_shift = 35; *y_shift = 0;
					break;
			}
			break;

		default:
			*width = 240; *height = 240; *x_shift = 0; *y_shift = 80;
			break;
	}
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void ST7789_setRotation(uint8_t m)
{
	// Update runtime configuration
	st7789_config.rotation = m;
	ST7789_CalculateDisplayParams(st7789_config.display_type, m,
	                               &st7789_config.width, &st7789_config.height,
	                               &st7789_config.x_shift, &st7789_config.y_shift);

	// Set hardware rotation
	ST7789_WriteCommand(ST7789_MADCTL);	// MADCTL
	switch (m) {
	case 0:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
		break;
	case 1:
		ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	case 2:
		ST7789_WriteSmallData(ST7789_MADCTL_RGB);
		break;
	case 3:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	default:
		break;
	}
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	ST7789_Select();
	uint16_t x_start = x0 + ST7789_X_SHIFT, x_end = x1 + ST7789_X_SHIFT;
	uint16_t y_start = y0 + ST7789_Y_SHIFT, y_end = y1 + ST7789_Y_SHIFT;

	/* Column Address set */
	ST7789_WriteCommand(ST7789_CASET);
	{
		uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}

	/* Row Address set */
	ST7789_WriteCommand(ST7789_RASET);
	{
		uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
		ST7789_WriteData(data, sizeof(data));
	}
	/* Write to RAM */
	ST7789_WriteCommand(ST7789_RAMWR);
	ST7789_UnSelect();
}

/**
 * @brief Initialize ST7789 controller with runtime parameters
 * @param display_type -> type of display (135x240, 240x240, or 170x320)
 * @param rotation -> rotation value (0-3)
 * @param buffer_size_bytes -> buffer size in bytes (0 = auto-calculate optimal size)
 *                             If non-zero but less than minimum, uses minimum size
 * @return none
 */
void ST7789_init(ST7789_DisplayType_t display_type, uint8_t rotation, uint16_t buffer_size_bytes)
{
	// Set display type and rotation
	st7789_config.display_type = display_type;
	st7789_config.rotation = rotation;

	// Calculate parameters internally
	ST7789_CalculateDisplayParams(display_type, rotation,
	                               &st7789_config.width, &st7789_config.height,
	                               &st7789_config.x_shift, &st7789_config.y_shift);

	// Allocate buffer with requested size
	// 0 = auto (optimal), non-zero = custom (bounded to min/max)
	ST7789_AllocateBuffer(buffer_size_bytes);

	// Hardware initialization
	HAL_Delay(10);
	ST7789_RST_Clr();
	HAL_Delay(10);
	ST7789_RST_Set();
	HAL_Delay(20);

	ST7789_WriteCommand(ST7789_COLMOD);		//	Set color mode
	ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
	ST7789_WriteCommand(ST7789_PORCTRL);	//	Porch control
	{
		uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
		ST7789_WriteData(data, sizeof(data));
	}
	ST7789_setRotation(st7789_config.rotation);	//	MADCTL (Display Rotation)

	/* Internal LCD Voltage generator settings */
	ST7789_WriteCommand(ST7789_GCTRL);		//	Gate Control
	ST7789_WriteSmallData(0x35);			//	Default value
	ST7789_WriteCommand(ST7789_VCOMS);		//	VCOM setting
	ST7789_WriteSmallData(0x19);			//	0.725v (default 0.75v for 0x20)
	ST7789_WriteCommand(ST7789_LCMCTRL);	//	LCM Control
	ST7789_WriteSmallData(0x2C);			//	Default value
	ST7789_WriteCommand(ST7789_VDVVRHEN);	//	VDV and VRH Command Enable
	ST7789_WriteSmallData(0x01);			//	Default value
	ST7789_WriteCommand(ST7789_VRHS);		//	VRH Set
	ST7789_WriteSmallData(0x12);			//	+-4.45v (default +-4.1v for 0x0B)
	ST7789_WriteCommand(ST7789_VDVS);		//	VDV Set
	ST7789_WriteSmallData(0x20);			//	Default value
	ST7789_WriteCommand(ST7789_FRCTRL2);	//	Frame Rate Control in Normal Mode
	ST7789_WriteSmallData(0x0F);			//	Default value (60Hz)
	ST7789_WriteCommand(ST7789_PWCTRL1);	//	Power Control 1
	ST7789_WriteSmallData(0xA4);			//	Default value
	ST7789_WriteSmallData(0xA1);			//	Default value
	/**************** Division line ****************/

	ST7789_WriteCommand(ST7789_PVGAMCTRL);	//	Positive Voltage Gamma Control
	{
		uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}

	ST7789_WriteCommand(ST7789_NVGAMCTRL);	//	Negative Voltage Gamma Control
	{
		uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
		ST7789_WriteData(data, sizeof(data));
	}
	ST7789_WriteCommand (ST7789_INVON);		//	Inversion ON
	ST7789_WriteCommand (ST7789_SLPOUT);	//	Out of sleep mode
	ST7789_WriteCommand (ST7789_NORON);		//	Normal Display on
	ST7789_WriteCommand (ST7789_DISPON);	//	Main screen turned on

	HAL_Delay(50);
	ST7789_fillScreen(ST7789_COLOR_BLACK);				//	Fill with Black.
}

/**
 * @brief Deinitialize ST7789 and free allocated buffer
 * @return none
 */
void ST7789_deinit(void)
{
	if (st7789_disp_buf != NULL) {
		free(st7789_disp_buf);
		st7789_disp_buf = NULL;
		st7789_disp_buf_size = 0;
	}
}

/**
 * @brief Get current display width
 * @return Current width in pixels
 */
uint16_t ST7789_width(void)
{
	return st7789_config.width;
}

/**
 * @brief Get current display height
 * @return Current height in pixels
 */
uint16_t ST7789_height(void)
{
	return st7789_config.height;
}

/**
 * @brief Get current rotation setting
 * @return Current rotation (0-3)
 */
uint8_t ST7789_getRotation(void)
{
	return st7789_config.rotation;
}

/**
 * @brief Get current display type
 * @return Display type enumeration value
 */
ST7789_DisplayType_t ST7789_getDisplayType(void)
{
	return st7789_config.display_type;
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_fillScreen(uint16_t color)
{
	ST7789_fillRect(0, 0, ST7789_WIDTH, ST7789_HEIGHT, color);
}

/**
 * @brief Internal helper to draw a pixel without CS control
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 * @note Caller must handle ST7789_Select/UnSelect
 */
static inline void ST7789_DrawPixel_Internal(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
		return;

	ST7789_SetAddressWindow(x, y, x, y);
	uint8_t data[] = {color >> 8, color & 0xFF};
	ST7789_WriteData(data, sizeof(data));
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_drawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	ST7789_Select();
	ST7789_DrawPixel_Internal(x, y, color);
	ST7789_UnSelect();
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point (center of 3x3 pixel block)
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
	// Need at least 1 pixel margin for 3x3 block
	if ((x < 1) || (x >= ST7789_WIDTH - 1) ||
	    (y < 1) || (y >= ST7789_HEIGHT - 1))
		return;

	ST7789_fillRect(x - 1, y - 1, 3, 3, color);
}

/**
 * @brief Internal helper to draw a line without CS control
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 * @note Caller must handle ST7789_Select/UnSelect
 */
static void ST7789_DrawLine_Internal(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	uint16_t swap;
    uint16_t steep = ST7789_ABS(y1 - y0) > ST7789_ABS(x1 - x0);
    if (steep) {
		swap = x0;
		x0 = y0;
		y0 = swap;

		swap = x1;
		x1 = y1;
		y1 = swap;
    }

    if (x0 > x1) {
		swap = x0;
		x0 = x1;
		x1 = swap;

		swap = y0;
		y0 = y1;
		y1 = swap;
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = ST7789_ABS(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            ST7789_DrawPixel_Internal(y0, x0, color);
        } else {
            ST7789_DrawPixel_Internal(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ST7789_drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    ST7789_Select();
    ST7789_DrawLine_Internal(x0, y0, x1, y1, color);
    ST7789_UnSelect();
}

/**
 * @brief Draw a fast horizontal line (optimized for axis-aligned lines)
 * @param x -> starting x coordinate
 * @param y -> y coordinate (constant for horizontal line)
 * @param w -> width (length of line in pixels)
 * @param color -> color of the line
 * @return none
 */
void ST7789_drawFastHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color)
{
	// Bounds checking
	if (y >= ST7789_HEIGHT) return;
	if (x >= ST7789_WIDTH) return;

	// Clip width to screen boundary
	if (x + w > ST7789_WIDTH) {
		w = ST7789_WIDTH - x;
	}

	if (w == 0) return;

	ST7789_Select();

	// Set address window for single row
	ST7789_SetAddressWindow(x, y, x + w - 1, y);

	// Fill line using buffer for efficiency
	uint16_t pixels_to_draw = w;
	uint16_t color_swapped = (color >> 8) | (color << 8);

	while (pixels_to_draw > 0) {
		uint16_t chunk = (pixels_to_draw > st7789_disp_buf_size) ? st7789_disp_buf_size : pixels_to_draw;

		// Fill buffer with color
		for (uint16_t i = 0; i < chunk; i++) {
			st7789_disp_buf[i] = color_swapped;
		}

		// Send chunk
		ST7789_WriteData((uint8_t*)st7789_disp_buf, chunk * 2);
		pixels_to_draw -= chunk;
	}

	ST7789_UnSelect();
}

/**
 * @brief Draw a fast vertical line (optimized for axis-aligned lines)
 * @param x -> x coordinate (constant for vertical line)
 * @param y -> starting y coordinate
 * @param h -> height (length of line in pixels)
 * @param color -> color of the line
 * @return none
 */
void ST7789_drawFastVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color)
{
	// Bounds checking
	if (x >= ST7789_WIDTH) return;
	if (y >= ST7789_HEIGHT) return;

	// Clip height to screen boundary
	if (y + h > ST7789_HEIGHT) {
		h = ST7789_HEIGHT - y;
	}

	if (h == 0) return;

	ST7789_Select();

	// Set address window for single column
	ST7789_SetAddressWindow(x, y, x, y + h - 1);

	// Fill line using buffer for efficiency
	uint16_t pixels_to_draw = h;
	uint16_t color_swapped = (color >> 8) | (color << 8);

	while (pixels_to_draw > 0) {
		uint16_t chunk = (pixels_to_draw > st7789_disp_buf_size) ? st7789_disp_buf_size : pixels_to_draw;

		// Fill buffer with color
		for (uint16_t i = 0; i < chunk; i++) {
			st7789_disp_buf[i] = color_swapped;
		}

		// Send chunk
		ST7789_WriteData((uint8_t*)st7789_disp_buf, chunk * 2);
		pixels_to_draw -= chunk;
	}

	ST7789_UnSelect();
}

/**
 * @brief Draw a Rectangle with single color
 * @param x, y -> top-left corner coordinates
 * @param w, h -> width and height
 * @param color -> color of the Rectangle line
 * @return none
 */
void ST7789_drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	if (w == 0 || h == 0) return;

	ST7789_Select();
	ST7789_DrawLine_Internal(x, y, x + w - 1, y, color);           // Top
	ST7789_DrawLine_Internal(x, y, x, y + h - 1, color);           // Left
	ST7789_DrawLine_Internal(x, y + h - 1, x + w - 1, y + h - 1, color); // Bottom
	ST7789_DrawLine_Internal(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
	ST7789_UnSelect();
}

/**
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ST7789_drawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_Select();
	ST7789_DrawPixel_Internal(x0, y0 + r, color);
	ST7789_DrawPixel_Internal(x0, y0 - r, color);
	ST7789_DrawPixel_Internal(x0 + r, y0, color);
	ST7789_DrawPixel_Internal(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawPixel_Internal(x0 + x, y0 + y, color);
		ST7789_DrawPixel_Internal(x0 - x, y0 + y, color);
		ST7789_DrawPixel_Internal(x0 + x, y0 - y, color);
		ST7789_DrawPixel_Internal(x0 - x, y0 - y, color);

		ST7789_DrawPixel_Internal(x0 + y, y0 + x, color);
		ST7789_DrawPixel_Internal(x0 - y, y0 + x, color);
		ST7789_DrawPixel_Internal(x0 + y, y0 - x, color);
		ST7789_DrawPixel_Internal(x0 - y, y0 - x, color);
	}
	ST7789_UnSelect();
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void ST7789_drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
	if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
		return;
	if ((x + w - 1) >= ST7789_WIDTH)
		return;
	if ((y + h - 1) >= ST7789_HEIGHT)
		return;

	ST7789_Select();
	ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);

	// Convert image data to big-endian format
	// Note: For large images, consider using a smaller temp buffer in chunks
	uint16_t pixel_count = w * h;
	for (uint16_t i = 0; i < pixel_count; i++) {
		uint16_t pixel = data[i];
		uint8_t pixel_data[] = {pixel >> 8, pixel & 0xFF};
		ST7789_WriteData(pixel_data, sizeof(pixel_data));
	}

	ST7789_UnSelect();
}

/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void ST7789_invertColors(uint8_t invert)
{
	ST7789_Select();
	ST7789_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
	ST7789_UnSelect();
}

/**
 * @brief Write a char using GFXfont format
 * @param  x&y -> cursor position (baseline)
 * @param ch -> char to write
 * @param font -> pointer to GFXfont structure
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST7789_drawChar(uint16_t x, uint16_t y, char ch, const GFXfont *font, uint16_t color, uint16_t bgcolor)
{
	// Check if character is in font range
	if ((ch < font->first) || (ch > font->last)) {
		return;
	}

	// Get glyph data
	GFXglyph *glyph = &font->glyph[ch - font->first];
	uint8_t *bitmap = font->bitmap;

	// Get glyph metrics
	uint16_t bo = glyph->bitmapOffset;
	uint8_t w = glyph->width;
	uint8_t h = glyph->height;
	int8_t xo = glyph->xOffset;
	int8_t yo = glyph->yOffset;

	// Calculate actual draw position
	int16_t draw_x = x + xo;
	int16_t draw_y = y + yo;

	// Bounds check - skip if completely outside screen
	if ((draw_x >= ST7789_WIDTH) || (draw_y >= ST7789_HEIGHT) ||
	    ((draw_x + w) <= 0) || ((draw_y + h) <= 0)) {
		return;
	}

	// Calculate clipped drawing region
	int16_t x_start = (draw_x < 0) ? 0 : draw_x;
	int16_t y_start = (draw_y < 0) ? 0 : draw_y;
	int16_t x_end = ((draw_x + w) > ST7789_WIDTH) ? (ST7789_WIDTH - 1) : (draw_x + w - 1);
	int16_t y_end = ((draw_y + h) > ST7789_HEIGHT) ? (ST7789_HEIGHT - 1) : (draw_y + h - 1);

	// Calculate actual drawing dimensions
	uint16_t draw_width = x_end - x_start + 1;
	uint16_t draw_height = y_end - y_start + 1;

	// Allocate buffer for character pixels (max typical glyph is ~30x30 = 900 pixels = 1800 bytes)
	// Use stack allocation for reasonable sizes, fall back to pixel-by-pixel for huge glyphs
	uint16_t pixel_count = draw_width * draw_height;

	if (pixel_count > 1600) {
		// Character too large for stack buffer - use slower method but still optimized
		// This should rarely happen with normal fonts
		ST7789_Select();
		ST7789_SetAddressWindow(x_start, y_start, x_end, y_end);

		for (uint16_t py = y_start; py <= y_end; py++) {
			for (uint16_t px = x_start; px <= x_end; px++) {
				// Calculate position in glyph bitmap
				int16_t glyph_x = px - draw_x;
				int16_t glyph_y = py - draw_y;

				// Calculate bit position in packed bitmap
				uint16_t bit_offset = bo * 8 + glyph_y * w + glyph_x;
				uint16_t byte_offset = bit_offset / 8;
				uint8_t bit_num = 7 - (bit_offset % 8);

				// Read bit from bitmap
				uint8_t bit = (bitmap[byte_offset] >> bit_num) & 0x01;

				// Send pixel color
				uint16_t pixel_color = bit ? color : bgcolor;
				uint8_t data[] = {pixel_color >> 8, pixel_color & 0xFF};
				ST7789_WriteData(data, sizeof(data));
			}
		}

		ST7789_UnSelect();
		return;
	}

	// Use stack buffer for efficient rendering
	uint16_t char_buffer[pixel_count];

	// Pre-calculate swapped colors for big-endian format
	uint16_t color_swapped = (color >> 8) | (color << 8);
	uint16_t bgcolor_swapped = (bgcolor >> 8) | (bgcolor << 8);

	// Fill buffer with character pixels
	uint16_t buf_idx = 0;
	for (uint16_t py = y_start; py <= y_end; py++) {
		for (uint16_t px = x_start; px <= x_end; px++) {
			// Calculate position in glyph bitmap
			int16_t glyph_x = px - draw_x;
			int16_t glyph_y = py - draw_y;

			// Calculate bit position in packed bitmap
			uint16_t bit_offset = bo * 8 + glyph_y * w + glyph_x;
			uint16_t byte_offset = bit_offset / 8;
			uint8_t bit_num = 7 - (bit_offset % 8);

			// Read bit from bitmap
			uint8_t bit = (bitmap[byte_offset] >> bit_num) & 0x01;

			// Set pixel color in buffer (big-endian format)
			char_buffer[buf_idx++] = bit ? color_swapped : bgcolor_swapped;
		}
	}

	// Send entire character in single transaction
	ST7789_Select();
	ST7789_SetAddressWindow(x_start, y_start, x_end, y_end);
	ST7789_WriteData((uint8_t*)char_buffer, pixel_count * 2);
	ST7789_UnSelect();
}

/**
 * @brief Write a string using GFXfont format
 * @param  x&y -> cursor position (baseline)
 * @param str -> string to write
 * @param font -> pointer to GFXfont structure
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789_drawString(uint16_t x, uint16_t y, const char *str, const GFXfont *font, uint16_t color, uint16_t bgcolor)
{
	int16_t cursor_x = x;
	int16_t cursor_y = y;

	while (*str) {
		char c = *str++;

		// Check if character is in font range
		if ((c < font->first) || (c > font->last)) {
			continue;
		}

		// Get glyph for this character
		GFXglyph *glyph = &font->glyph[c - font->first];

		// Check for line wrap
		if (cursor_x + glyph->xOffset + glyph->width >= ST7789_WIDTH) {
			cursor_x = 0;
			cursor_y += font->yAdvance;

			// Stop if we've gone off bottom of screen
			if (cursor_y >= ST7789_HEIGHT) {
				break;
			}

			// Skip spaces at beginning of new line
			if (c == ' ') {
				continue;
			}
		}

		// Draw the character
		ST7789_drawChar(cursor_x, cursor_y, c, font, color, bgcolor);

		// Advance cursor
		cursor_x += glyph->xAdvance;
	}
}

/**
 * @brief Calculate bounding box for a text string
 * @param str -> string to measure
 * @param font -> pointer to GFX font
 * @param w -> pointer to store calculated width
 * @param h -> pointer to store calculated height
 * @return none
 * @note This function only calculates dimensions without rendering
 */
void ST7789_getTextBounds(const char *str, const GFXfont *font, uint16_t *w, uint16_t *h)
{
	if (str == NULL || font == NULL || w == NULL || h == NULL) {
		return;
	}

	int16_t cursor_x = 0;
	int16_t min_x = 0, max_x = 0;
	int16_t min_y = 0, max_y = 0;
	uint8_t first_char = 1;

	while (*str) {
		char c = *str++;

		// Check if character is in font range
		if ((c < font->first) || (c > font->last)) {
			continue;
		}

		// Get glyph for this character
		GFXglyph *glyph = &font->glyph[c - font->first];

		if (first_char) {
			// Initialize bounds with first character
			min_x = cursor_x + glyph->xOffset;
			max_x = min_x + glyph->width;
			min_y = glyph->yOffset;
			max_y = min_y + glyph->height;
			first_char = 0;
		} else {
			// Expand bounds for subsequent characters
			int16_t char_min_x = cursor_x + glyph->xOffset;
			int16_t char_max_x = char_min_x + glyph->width;
			int16_t char_min_y = glyph->yOffset;
			int16_t char_max_y = char_min_y + glyph->height;

			if (char_min_x < min_x) min_x = char_min_x;
			if (char_max_x > max_x) max_x = char_max_x;
			if (char_min_y < min_y) min_y = char_min_y;
			if (char_max_y > max_y) max_y = char_max_y;
		}

		// Advance cursor
		cursor_x += glyph->xAdvance;
	}

	// Calculate final dimensions
	*w = (uint16_t)(max_x - min_x);
	*h = (uint16_t)(max_y - min_y);
}

/**
 * @brief Draw a filled Rectangle with single color
 * @param  x&y -> coordinates of the starting point
 * @param w&h -> width & height of the Rectangle
 * @param color -> color of the Rectangle
 * @return  none
 */
void ST7789_fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	uint32_t size;
	uint32_t buffer_bytes;
	uint32_t buffer_count;
	uint32_t remainder;
	uint16_t color_swapped;
	uint16_t i;
	uint32_t j;

	/* Check input parameters */
	if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
		return;
	}

	/* Clip width and height to screen boundaries */
	if ((x + w) > ST7789_WIDTH) {
		w = ST7789_WIDTH - x;
	}
	if ((y + h) > ST7789_HEIGHT) {
		h = ST7789_HEIGHT - y;
	}

	if (w == 0 || h == 0) {
		return;
	}

	ST7789_Select();

	/* Calculate total size in bytes */
	size = w * h * 2;
	buffer_bytes = st7789_disp_buf_size * sizeof(uint16_t);
	buffer_count = size / buffer_bytes;
	remainder = size % buffer_bytes;

	/* Fill buffer with color in big-endian format */
	color_swapped = (color >> 8) | (color << 8);
	for (i = 0; i < st7789_disp_buf_size; i++) {
		st7789_disp_buf[i] = color_swapped;
	}

	/* Set address window and write data */
	ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);
	for (j = 0; j < buffer_count; j++) {
		ST7789_WriteData((uint8_t*)st7789_disp_buf, buffer_bytes);
	}

	if (remainder > 0) {
		ST7789_WriteData((uint8_t*)st7789_disp_buf, remainder);
	}

	ST7789_UnSelect();
}

/**
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ST7789_drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	ST7789_Select();
	/* Draw lines */
	ST7789_DrawLine_Internal(x1, y1, x2, y2, color);
	ST7789_DrawLine_Internal(x2, y2, x3, y3, color);
	ST7789_DrawLine_Internal(x3, y3, x1, y1, color);
	ST7789_UnSelect();
}

/**
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ST7789_fillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
	ST7789_Select();
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
			yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
			curpixel = 0;

	deltax = ST7789_ABS(x2 - x1);
	deltay = ST7789_ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	}
	else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	}
	else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay) {
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	}
	else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		ST7789_DrawLine_Internal(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
	ST7789_UnSelect();
}

/**
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ST7789_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	ST7789_Select();
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	ST7789_DrawPixel_Internal(x0, y0 + r, color);
	ST7789_DrawPixel_Internal(x0, y0 - r, color);
	ST7789_DrawPixel_Internal(x0 + r, y0, color);
	ST7789_DrawPixel_Internal(x0 - r, y0, color);
	ST7789_DrawLine_Internal(x0 - r, y0, x0 + r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7789_DrawLine_Internal(x0 - x, y0 + y, x0 + x, y0 + y, color);
		ST7789_DrawLine_Internal(x0 + x, y0 - y, x0 - x, y0 - y, color);

		ST7789_DrawLine_Internal(x0 + y, y0 + x, x0 - y, y0 + x, color);
		ST7789_DrawLine_Internal(x0 + y, y0 - x, x0 - y, y0 - x, color);
	}
	ST7789_UnSelect();
}


/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void ST7789_tearEffect(uint8_t tear)
{
	ST7789_Select();
	ST7789_WriteCommand(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
	ST7789_UnSelect();
}


/**
 * @brief A Simple test function for ST7789
 * @param  none
 * @return  none
 */
void ST7789_test(void)
{
	ST7789_fillScreen(ST7789_COLOR_WHITE);
	ST7789_drawString(10, 30, "Speed Test", &FreeSans12pt7b, ST7789_COLOR_RED, ST7789_COLOR_WHITE);
	ST7789_fillScreen(ST7789_COLOR_CYAN);
	ST7789_fillScreen(ST7789_COLOR_RED);
	ST7789_fillScreen(ST7789_COLOR_BLUE);
	ST7789_fillScreen(ST7789_COLOR_GREEN);
	ST7789_fillScreen(ST7789_COLOR_YELLOW);
	ST7789_fillScreen(ST7789_COLOR_BROWN);
	ST7789_fillScreen(ST7789_COLOR_DARKBLUE);
	ST7789_fillScreen(ST7789_COLOR_MAGENTA);
	ST7789_fillScreen(ST7789_COLOR_LIGHTGREEN);
	ST7789_fillScreen(ST7789_COLOR_LGRAY);
	ST7789_fillScreen(ST7789_COLOR_LBBLUE);
	ST7789_fillScreen(ST7789_COLOR_WHITE);

	ST7789_drawString(10, 30, "Font test.", &FreeSerifBold18pt7b, ST7789_COLOR_GBLUE, ST7789_COLOR_WHITE);
	ST7789_drawString(10, 60, "Hello World!", &FreeSans12pt7b, ST7789_COLOR_RED, ST7789_COLOR_WHITE);
	ST7789_drawString(10, 90, "GFX Fonts", &FreeMono9pt7b, ST7789_COLOR_YELLOW, ST7789_COLOR_WHITE);
	ST7789_drawString(10, 120, "STM32 Demo", &Org_01, ST7789_COLOR_MAGENTA, ST7789_COLOR_WHITE);

	ST7789_fillScreen(ST7789_COLOR_RED);
	ST7789_drawString(10, 30, "Rect./Line.", &FreeSans12pt7b, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
	ST7789_drawRect(30, 40, 71, 61, ST7789_COLOR_WHITE);  // 30,40,100,100 corners -> 30,40,71x61 size

	ST7789_fillScreen(ST7789_COLOR_RED);
	ST7789_drawString(10, 30, "Filled Rect.", &FreeSans12pt7b, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
	ST7789_fillRect(30, 40, 50, 50, ST7789_COLOR_WHITE);

	ST7789_fillScreen(ST7789_COLOR_RED);
	ST7789_drawString(10, 30, "Circle.", &FreeSans12pt7b, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
	ST7789_drawCircle(60, 80, 25, ST7789_COLOR_WHITE);

	ST7789_fillScreen(ST7789_COLOR_RED);
	ST7789_drawString(10, 30, "Filled Cir.", &FreeSans12pt7b, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
	ST7789_fillCircle(60, 80, 25, ST7789_COLOR_WHITE);

	ST7789_fillScreen(ST7789_COLOR_RED);
	ST7789_drawString(10, 30, "Triangle", &FreeSans12pt7b, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
	ST7789_drawTriangle(30, 50, 30, 90, 60, 60, ST7789_COLOR_WHITE);

	ST7789_fillScreen(ST7789_COLOR_RED);
	ST7789_drawString(10, 30, "Filled Tri", &FreeSans12pt7b, ST7789_COLOR_YELLOW, ST7789_COLOR_BLACK);
	ST7789_fillTriangle(30, 50, 30, 90, 60, 60, ST7789_COLOR_WHITE);
}
