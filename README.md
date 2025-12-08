# ST7789-STM32
Using STM32's Hardware SPI (with DMA support) to drive a ST7789 based LCD display.

## How to use

1. Copy the "st7789" dir to your project src path, add it to include path
2. Include `"st7789.h"` in where you want to use this driver.
3. Configure pin definitions in `"st7789.h"` (RST, DC, CS pins)
4. In system startup, initialize with `ST7789_init(display_type, rotation, buffer_size);`
5. Run `ST7789_test()` to verify the driver
6. Don't forget to turn the backlight on

### Initialization Example

```c
// Initialize with 240x240 display, rotation 0, 2400 byte buffer
ST7789_init(ST7789_DISPLAY_240x240, 0, 2400);

// Get display dimensions
uint16_t w = ST7789_width();
uint16_t h = ST7789_height();
```  

This code has been tested on 240x240 & 170x320 LCD screens.

> DMA is only useful when huge block write is performed, e.g: Fill full screen or draw a bitmap.
> Most MCUs don't have a large enough RAM, so a framebuffer is "cut" into pieces, e.g: a 240x5 pixel buffer for a 240x240 screen.

## API Reference

### Initialization & Configuration
- `ST7789_init(display_type, rotation, buffer_size)` - Initialize display
- `ST7789_deinit()` - Deinitialize and free resources
- `ST7789_setRotation(rotation)` - Change screen rotation (0-3)
- `ST7789_width()` - Get current width in pixels
- `ST7789_height()` - Get current height in pixels
- `ST7789_getRotation()` - Get current rotation
- `ST7789_getDisplayType()` - Get display type

### Basic Drawing
- `ST7789_fillScreen(color)` - Fill entire screen with color
- `ST7789_drawPixel(x, y, color)` - Draw single pixel
- `ST7789_fillRect(x, y, w, h, color)` - Draw filled rectangle

### Shapes
- `ST7789_drawLine(x0, y0, x1, y1, color)` - Draw line
- `ST7789_drawFastHLine(x, y, w, color)` - Draw fast horizontal line
- `ST7789_drawFastVLine(x, y, h, color)` - Draw fast vertical line
- `ST7789_drawRect(x, y, w, h, color)` - Draw rectangle outline
- `ST7789_drawCircle(x, y, r, color)` - Draw circle outline
- `ST7789_fillCircle(x, y, r, color)` - Draw filled circle
- `ST7789_drawTriangle(x1, y1, x2, y2, x3, y3, color)` - Draw triangle outline
- `ST7789_fillTriangle(x1, y1, x2, y2, x3, y3, color)` - Draw filled triangle

### Text (Adafruit GFX Fonts)
- `ST7789_drawChar(x, y, ch, font, color, bgcolor)` - Draw single character
- `ST7789_drawString(x, y, str, font, color, bgcolor)` - Draw text string
- `ST7789_getTextBounds(str, font, &w, &h)` - Calculate text dimensions

### Images & Utilities
- `ST7789_drawImage(x, y, w, h, data)` - Draw RGB565 bitmap
- `ST7789_invertColors(invert)` - Invert display colors
- `ST7789_tearEffect(enable)` - Enable/disable tearing effect line
- `ST7789_test()` - Run demonstration/test  

## SPI Interface

If you are using **Dupont Line(or jumper wire)**, please notice that your CLK frequency should not exceed 40MHz (may vary, depends on the length of your wire), **otherwise data transfer will collapse!**  
For higher speed applications, it's recommended to **use PCB** rather than jumper wires.  

In STM32CubeMX/CubeIDE, config the SPI params as follow:

![spi](fig/spi.jpg)

I've had a simple test, connect the screen and mcu via 20cm dupont line, and it works normally on **21.25MB/s**. And if I connect a logic analyzer to the clk and data lines(15cm probe), **21.25MB/s doesn't work anymore**, I have to lower its datarate to 10.625MB/s. Using PCB to connect the display, it works up to **40MB/s** and still looks nice.

## Supported Displays

- 135*240   
- 240*240   
- 170*320 (new)  

If you like, you could customize it's resolution to drive different displays you prefer. 
> For example, a 240x320 display is perfectly suited for st7789.  
> Just set all X_SHIFT and Y_SHIFT to 0, and set resolution to 240|320.  

For more details, please refer to ST7789's datasheet.  

## HAL SPI Performance

- DMA Enabled

With DMA enabled, cpu won't participate in the data transfer process. So filling a large size of data block is much faster.e.g. fill, drawImage. (You can see no interval between each data write)

![DMA](/fig/fill_dma.png)


- DMA Disabled

Without DMA enabled, the filling process could be a suffer. As you can see, before each data byte write, an interval is inserted, so the total datarate would degrade. 

![noDMA](/fig/fill_normal.png)

Especially in some functions where need a little math, the cpu needs to calculate data before a write operation, so the effective datarate would be much lower.(e.g. drawLine)

![line](fig/draw_line.png)


# Special thanks to

#### Reference
- [ananevilya's Arduino-ST7789-Lib](https://github.com/ananevilya/Arduino-ST7789-Library)  
- [afiskon's stm32-st7735 lib](https://github.com/afiskon/stm32-st7735)

#### Contributor
- [JasonLrh](https://github.com/JasonLrh)  
- [ZiangCheng](https://github.com/ZiangCheng)  
