#pragma once

#include <stdint.h>

typedef struct
{
    const uint8_t *bitmap;  // Bitmap data (1-bit per pixel)
    uint16_t width;         // Width in pixels
    uint16_t height;        // Height in pixels
} Shape;
