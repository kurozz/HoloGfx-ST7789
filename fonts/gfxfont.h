#ifndef _GFXFONT_H_
#define _GFXFONT_H_

#include <stdint.h>

/**
 * @brief Glyph structure for individual character metrics
 *
 * Structure describing a single character glyph in the font.
 * Based on Adafruit GFX Library format.
 */
typedef struct {
    uint16_t bitmapOffset; // Pointer into GFXfont->bitmap
    uint8_t width;         // Bitmap dimensions in pixels
    uint8_t height;        // Bitmap dimensions in pixels
    uint8_t xAdvance;      // Distance to advance cursor (x axis)
    int8_t xOffset;        // X dist from cursor pos to UL corner
    int8_t yOffset;        // Y dist from cursor pos to UL corner
} GFXglyph;

/**
 * @brief Font structure for complete font data
 *
 * Data stored for FONT AS A WHOLE
 * Based on Adafruit GFX Library format.
 */
typedef struct {
    uint8_t *bitmap;  // Glyph bitmaps, concatenated (1-bit packed)
    GFXglyph *glyph;  // Glyph array
    uint16_t first;   // ASCII extents (first char)
    uint16_t last;    // ASCII extents (last char)
    uint8_t yAdvance; // Newline distance (y axis)
} GFXfont;

#endif // _GFXFONT_H_
