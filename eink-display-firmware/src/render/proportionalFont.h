#pragma once

#include <stdint.h>

// Extends the basic Shape struct to support variable-width characters.
typedef struct
{
    const uint8_t *bitmap;   // Bitmap data (1-bit per pixel)
    const uint16_t *widths;  // Array of character widths
    const uint32_t *offsets; // Array of byte offsets into bitmap for each char
    uint16_t maxWidth;       // Maximum character width
    uint16_t height;         // Height in pixels
    uint8_t firstChar;       // ASCII code of first character (usually 32 for space)
} ProportionalFont;
