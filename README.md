# HoloGfx-ST7789

ST7789 TFT LCD driver for STM32 microcontrollers using HAL SPI with optional DMA support.

---

## Features

- **DMA Support**: Optional DMA transfers for increased performance
- **Configurable Buffer**: Dynamic allocation from 256 bytes up to full framebuffer or 65535 bytes
- **Adafruit GFX Fonts**: Compatibility with Adafruit GFX font format

---

## Supported Platforms

### STM32 Families
Compatible with all STM32 families using generic HAL:
- STM32F0, F1, F4, F7
- STM32H7
- STM32L4
- STM32G0

**Tested on**: STM32F103, STM32G071

### Display Types
- 135×240 pixels (not tested)
- 240×240 pixels
- 170×320 pixels (not tested)

> Custom resolutions can be configured. Refer to ST7789 datasheet for details.

---

## Getting Started

### Installation
TODO: Add installation instructions

### Hardware Setup
TODO: Add wiring diagram and pin configuration

### CubeMX Configuration
TODO: Add SPI and DMA configuration steps

### Basic Usage

```c
#include "st7789.h"

// Initialize display
ST7789_Status_t status = ST7789_init(ST7789_DISPLAY_240x240, 0, 2400);
if (status != ST7789_OK) {
    // Handle initialization error
}

// Fill screen with color
ST7789_fillScreen(ST7789_COLOR_BLACK);

// Draw text
ST7789_drawString(10, 10, "Hello World!", &FreeSans12pt7b, ST7789_COLOR_WHITE, ST7789_COLOR_BLACK);
```

---

## Adafruit GFX Font Format

This library uses the Adafruit GFX font format for text rendering, providing compatibility with the extensive collection of fonts available in the [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library).

### Adding Custom Fonts

1. Convert your font using [Adafruit GFX fontconvert](https://github.com/adafruit/Adafruit-GFX-Library/tree/master/fontconvert)
2. Add the generated `.h` file to the `fonts/` directory
3. Change the include guard from `#pragma once` type to `#ifndef` type
4. Change `#include <Adafruit_GFX.h>` to `#include "gfxfont.h"`
5. Change types from `const uint8_t FreeMono12pt7bBitmaps[] PROGMEM` or similar to `static const uint8_t FreeMono12pt7bBitmaps[]`
6. Include the font header in `fonts/fonts.h`

---

## Credits & References

This library is based on and inspired by:

- **[ST7789-STM32](https://github.com/Floyd-Fish/ST7789-STM32)** by Floyd-Fish - Base ST7789 driver implementation
- **[Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)** - Font format and text rendering concepts
- [Arduino-ST7789-Library](https://github.com/ananevilya/Arduino-ST7789-Library) by ananevilya
- [stm32-st7735](https://github.com/afiskon/stm32-st7735) by afiskon

---

## License

This project is licensed under the **GNU General Public License v3.0** (GPL-3.0).

See the [LICENSE](LICENSE) file for details.
