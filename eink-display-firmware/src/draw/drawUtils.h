#pragma once

#include <cstdint>

#include "color.h"
#include "shape.h"
#include "proportionalFont.h"

class DrawUtils
{
public:
    static void clearImage(uint8_t *imageData);
    static void setPixel(uint8_t *imageData, uint16_t x, uint16_t y, Color color);

    static void drawLine(uint8_t *imageData, uint16_t xStart, uint16_t xEnd, uint16_t yStart, uint16_t yEnd, Color color);
    static void drawRectangle(uint8_t *imageData, uint16_t xStart, uint16_t xEnd, uint16_t yStart, uint16_t yEnd, Color color, bool fill);

    // Monospace font functions. Returns the width of the drawn string
    static uint16_t drawString(uint8_t *imageData, uint16_t x, uint16_t y, const char *str, const Shape *font, Color color);
    static uint16_t drawChar(uint8_t *imageData, uint16_t x, uint16_t y, char c, const Shape *font, Color color);

    // Proportional font functions. Returns the width of the drawn string
    static uint16_t drawStringProp(uint8_t *imageData, uint16_t x, uint16_t y, const char *str, const ProportionalFont *font, Color color);
    static uint16_t drawCharProp(uint8_t *imageData, uint16_t x, uint16_t y, char c, const ProportionalFont *font, Color color);
    static uint16_t getStringWidthProp(const char *str, const ProportionalFont *font);

    static uint16_t drawShape(uint8_t *imageData, uint16_t x, uint16_t y, const Shape *shape, Color color);
};