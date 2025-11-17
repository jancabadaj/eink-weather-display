#pragma once

#include <cstdint>

#include "color.h"
#include "shape.h"

class DrawUtils
{
public:
    static void clearImage(uint8_t *imageData);
    static void setPixel(uint8_t *imageData, uint16_t x, uint16_t y, Color color);

    static void drawLine(uint8_t *imageData, uint16_t xStart, uint16_t xEnd, uint16_t yStart, uint16_t yEnd, Color color);
    static void drawRectangle(uint8_t *imageData, uint16_t xStart, uint16_t xEnd, uint16_t yStart, uint16_t yEnd, Color color, bool fill);

    static void drawString(uint8_t *imageData, uint16_t x, uint16_t y, const char *str, const Shape *font, Color color);
    static void drawChar(uint8_t *imageData, uint16_t x, uint16_t y, char c, const Shape *font, Color color);

    static void drawIcon(uint8_t *imageData, uint16_t x, uint16_t y, const Shape *icon, Color color);
};