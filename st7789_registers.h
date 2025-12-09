#ifndef __ST7789_REGISTERS_H
#define __ST7789_REGISTERS_H

/**
 * @file st7789_registers.h
 * @brief ST7789 display controller register definitions
 *
 * Based on ST7789V datasheet
 * All commands are 8-bit values sent via SPI
 */

/* ============================================================================
 * Basic Commands
 * ============================================================================ */
#define ST7789_NOP     0x00  /* No Operation */
#define ST7789_SWRESET 0x01  /* Software Reset */
#define ST7789_RDDID   0x04  /* Read Display ID */
#define ST7789_RDDST   0x09  /* Read Display Status */

/* ============================================================================
 * Sleep Control
 * ============================================================================ */
#define ST7789_SLPIN   0x10  /* Sleep In */
#define ST7789_SLPOUT  0x11  /* Sleep Out */

/* ============================================================================
 * Partial Mode Control
 * ============================================================================ */
#define ST7789_PTLON   0x12  /* Partial Display Mode On */
#define ST7789_NORON   0x13  /* Normal Display Mode On */

/* ============================================================================
 * Display Inversion Control
 * ============================================================================ */
#define ST7789_INVOFF  0x20  /* Display Inversion Off */
#define ST7789_INVON   0x21  /* Display Inversion On */

/* ============================================================================
 * Display Control
 * ============================================================================ */
#define ST7789_DISPOFF 0x28  /* Display Off */
#define ST7789_DISPON  0x29  /* Display On */

/* ============================================================================
 * Address and Data Control
 * ============================================================================ */
#define ST7789_CASET   0x2A  /* Column Address Set */
#define ST7789_RASET   0x2B  /* Row Address Set */
#define ST7789_RAMWR   0x2C  /* Memory Write */
#define ST7789_RAMRD   0x2E  /* Memory Read */

/* ============================================================================
 * Partial Area Control
 * ============================================================================ */
#define ST7789_PTLAR   0x30  /* Partial Area */

/* ============================================================================
 * Tearing Effect Control
 * ============================================================================ */
#define ST7789_TEOFF   0x34  /* Tearing Effect Line Off */
#define ST7789_TEON    0x35  /* Tearing Effect Line On */

/* ============================================================================
 * Memory Access Control
 * ============================================================================ */
#define ST7789_MADCTL  0x36  /* Memory Data Access Control */
#define ST7789_COLMOD  0x3A  /* Interface Pixel Format */

/* ============================================================================
 * Frame Rate Control
 * ============================================================================ */
#define ST7789_RAMCTRL 0xB0  /* RAM Control */
#define ST7789_RGBCTRL 0xB1  /* RGB Interface Control */
#define ST7789_PORCTRL 0xB2  /* Porch Control */
#define ST7789_FRCTRL1 0xB3  /* Frame Rate Control 1 (In partial mode/idle colors) */

/* ============================================================================
 * Gamma Control
 * ============================================================================ */
#define ST7789_GCTRL   0xB7  /* Gate Control */
#define ST7789_GTADJ   0xB8  /* Gate On Timing Adjustment */
#define ST7789_DGMEN   0xBA  /* Digital Gamma Enable */

/* ============================================================================
 * Power Control
 * ============================================================================ */
#define ST7789_VCOMS   0xBB  /* VCOM Setting */
#define ST7789_LCMCTRL 0xC0  /* LCM Control */
#define ST7789_IDSET   0xC1  /* ID Code Setting */
#define ST7789_VDVVRHEN 0xC2 /* VDV and VRH Command Enable */
#define ST7789_VRHS    0xC3  /* VRH Set */
#define ST7789_VDVS    0xC4  /* VDV Set */
#define ST7789_VCMOFSET 0xC5 /* VCOM Offset Set */
#define ST7789_FRCTRL2  0xC6  /* Frame Rate Control in Normal Mode */
#define ST7789_CABCCTRL 0xC7 /* CABC Control */
#define ST7789_REGSEL1 0xC8  /* Register Value Selection 1 */
#define ST7789_REGSEL2 0xCA  /* Register Value Selection 2 */

/* ============================================================================
 * Power Control (continued)
 * ============================================================================ */
#define ST7789_PWMFRSEL 0xCC /* PWM Frequency Selection */
#define ST7789_PWCTRL1 0xD0  /* Power Control 1 */
#define ST7789_VAPVANEN 0xD2 /* Enable VAP/VAN signal output */

/* ============================================================================
 * Command Set Control
 * ============================================================================ */
#define ST7789_CMD2EN  0xDF  /* Command 2 Enable */

/* ============================================================================
 * Gamma Control (Positive/Negative Polarity)
 * ============================================================================ */
#define ST7789_PVGAMCTRL 0xE0 /* Positive Voltage Gamma Control */
#define ST7789_NVGAMCTRL 0xE1 /* Negative Voltage Gamma Control */

/* ============================================================================
 * Digital Gamma Control
 * ============================================================================ */
#define ST7789_DGMLUTR 0xE2  /* Digital Gamma Look-up Table for Red */
#define ST7789_DGMLUTB 0xE3  /* Digital Gamma Look-up Table for Blue */

/* ============================================================================
 * Gate Control
 * ============================================================================ */
#define ST7789_GATECTRL 0xE4 /* Gate Control */

/* ============================================================================
 * SPI2 Enable
 * ============================================================================ */
#define ST7789_SPI2EN  0xE7  /* SPI2 Enable */

/* ============================================================================
 * Power Control (continued)
 * ============================================================================ */
#define ST7789_PWCTRL2 0xE8  /* Power Control 2 */
#define ST7789_EQCTRL  0xE9  /* Equalize Time Control */
#define ST7789_PROMCTRL 0xEC /* Program Mode Control */
#define ST7789_PROMEN  0xFA  /* Program Mode Enable */
#define ST7789_NVMSET  0xFC  /* NVM Setting */
#define ST7789_PROMACT 0xFE  /* Program Action */

/* ============================================================================
 * Read ID Registers
 * ============================================================================ */
#define ST7789_RDID1   0xDA  /* Read ID1 */
#define ST7789_RDID2   0xDB  /* Read ID2 */
#define ST7789_RDID3   0xDC  /* Read ID3 */
#define ST7789_RDID4   0xDD  /* Read ID4 */

#endif /* __ST7789_REGISTERS_H */
