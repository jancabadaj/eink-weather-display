#include "drawUtils.h"
#include "../definitions.h"

#define COLOR_WHITE 0xFF
#define COLOR_BLACK 0x00

#define pgmc_read_byte(addr) (*(const unsigned char *)(addr))
#define pgmc_read_word(addr) ({       \
    typeof(addr) _addr = (addr);      \
    *(const unsigned short *)(_addr); \
})

// Internal helper template for string drawing
// GetWidthFunc: function that takes (char c) and returns uint16_t width
// DrawCharFunc: function that takes (uint16_t x, uint16_t y, char c)
template <typename FontType, typename GetWidthFunc, typename DrawCharFunc>
static uint16_t drawStringHelper(uint8_t *imageData, uint16_t xStart, uint16_t y, const char *str,
                                 const FontType *font, Color color, uint16_t fontHeight,
                                 GetWidthFunc getWidth, DrawCharFunc drawCharFunc)
{
    if (xStart > IMAGE_WIDTH || y > IMAGE_HEIGHT)
    {
        return 0; // Starting point out of bounds
    }

    if ((y + fontHeight) > IMAGE_HEIGHT)
    {
        return 0; // Height exceeds image bounds
    }

    uint16_t xCurrent = xStart;

    while (*str != '\0')
    {
        uint16_t charWidth = getWidth(*str);

        if ((xCurrent + charWidth) > IMAGE_WIDTH)
        {
            return xCurrent - xStart; // String exceeds image width
        }

        drawCharFunc(xCurrent, y, *str);

        xCurrent += charWidth;
        str++;
    }

    return (xCurrent - xStart);
}

void DrawUtils::clearImage(uint8_t *imageData)
{
    for (uint16_t y = 0; y < IMAGE_HEIGHT_BYTE; y++)
    {
        for (uint16_t x = 0; x < IMAGE_WIDTH_BYTE; x++) // 8 pixel =  1 byte
        {
            uint32_t addr = x + y * IMAGE_WIDTH_BYTE;
            imageData[addr] = COLOR_WHITE;
        }
    }
}

void DrawUtils::setPixel(uint8_t *imageData, uint16_t x, uint16_t y, Color color)
{
    if (x > IMAGE_WIDTH || y > IMAGE_HEIGHT)
    {
        return;
    }

    uint32_t addr = x / 8 + y * IMAGE_WIDTH_BYTE;
    uint8_t rData = imageData[addr];
    if (color == Black)
        imageData[addr] = rData & ~(0x80 >> (x % 8));
    else
        imageData[addr] = rData | (0x80 >> (x % 8));
}

void DrawUtils::drawLine(uint8_t *imageData, uint16_t xStart, uint16_t xEnd, uint16_t yStart, uint16_t yEnd, Color color)
{
    if (xStart > IMAGE_WIDTH || yStart > IMAGE_HEIGHT ||
        xEnd > IMAGE_WIDTH || yEnd > IMAGE_HEIGHT)
    {
        return;
    }

    uint16_t x = xStart;
    uint16_t y = yStart;
    int dx = (int)xEnd - (int)xStart >= 0 ? xEnd - xStart : xStart - xEnd;
    int dy = (int)yEnd - (int)yStart <= 0 ? yEnd - yStart : yStart - yEnd;

    int xIncrement = xStart < xEnd ? 1 : -1;
    int yIncrement = yStart < yEnd ? 1 : -1;

    int err = dx + dy;
    while (x != xEnd || y != yEnd)
    {
        setPixel(imageData, x, y, color);

        if (2 * err >= dy)
        {
            err += dy;
            x += xIncrement;
        }
        if (2 * err <= dx)
        {
            err += dx;
            y += yIncrement;
        }
    }
}

void DrawUtils::drawRectangle(uint8_t *imageData, uint16_t xStart, uint16_t xEnd, uint16_t yStart, uint16_t yEnd, Color color, bool fill)
{
    if (xStart > IMAGE_WIDTH || yStart > IMAGE_HEIGHT ||
        xEnd > IMAGE_WIDTH || yEnd > IMAGE_HEIGHT)
    {
        return;
    }

    if (fill)
    {
        uint16_t y;
        for (y = yStart; y < yEnd; y++)
        {
            DrawUtils::drawLine(imageData, xStart, xEnd, y, y, color);
        }
    }
    else
    {
        DrawUtils::drawLine(imageData, xStart, xEnd, yStart, yStart, color);
        DrawUtils::drawLine(imageData, xStart, xStart, yStart, yEnd, color);
        DrawUtils::drawLine(imageData, xEnd, xEnd, yEnd, yStart, color);
        DrawUtils::drawLine(imageData, xEnd, xStart, yEnd, yEnd, color);
    }
}

uint16_t DrawUtils::drawShape(uint8_t *imageData, uint16_t x, uint16_t y, const Shape *shape, Color color)
{
    if (x > IMAGE_WIDTH || y > IMAGE_HEIGHT)
    {
        return 0; // Starting point out of bounds
    }

    if ((x + shape->width) > IMAGE_WIDTH || (y + shape->height) > IMAGE_HEIGHT)
    {
        return 0; // Exceeds image bounds
    }

    const unsigned char *ptr = shape->bitmap;

    for (uint16_t row = 0; row < shape->height; row++)
    {
        for (uint16_t col = 0; col < shape->width; col++)
        {
            if (*ptr & (0x80 >> (col % 8)))
                setPixel(imageData, x + col, y + row, color);

            if (col % 8 == 7)
                ptr++;
        }

        if (shape->width % 8 != 0)
            ptr++;
    }

    return shape->width;
}

uint16_t DrawUtils::drawString(uint8_t *imageData, uint16_t x, uint16_t y, const char *str, const Shape *font, Color color)
{
    auto getWidth = [&](char c) -> uint16_t
    { return font->width; };
    auto drawCharFunc = [&](uint16_t cx, uint16_t cy, char c)
    {
        drawChar(imageData, cx, cy, c, font, color);
    };

    return drawStringHelper(imageData, x, y, str, font, color, font->height, getWidth, drawCharFunc);
}

uint16_t DrawUtils::drawChar(uint8_t *imageData, uint16_t x, uint16_t y, const char c, const Shape *font, Color color)
{
    uint32_t charOffset = (c - ' ') * font->height * (font->width / 8 + (font->width % 8 ? 1 : 0));

    Shape charShape{&font->bitmap[charOffset], font->width, font->height};
    return drawShape(imageData, x, y, &charShape, color);
}

uint16_t DrawUtils::drawStringProp(uint8_t *imageData, uint16_t x, uint16_t y, const char *str, const ProportionalFont *font, Color color)
{
    auto getWidth = [&](char c) -> uint16_t
    {
        uint8_t charIndex = c - font->firstChar;
        return font->widths[charIndex];
    };
    auto drawCharFunc = [&](uint16_t cx, uint16_t cy, char c)
    {
        drawCharProp(imageData, cx, cy, c, font, color);
    };

    return drawStringHelper(imageData, x, y, str, font, color, font->height, getWidth, drawCharFunc);
}

uint16_t DrawUtils::drawCharProp(uint8_t *imageData, uint16_t x, uint16_t y, char c, const ProportionalFont *font, Color color)
{
    uint8_t charIndex = c - font->firstChar;
    uint16_t charWidth = font->widths[charIndex];
    uint32_t charOffset = font->offsets[charIndex];

    Shape charShape{&font->bitmap[charOffset], charWidth, font->height};
    return drawShape(imageData, x, y, &charShape, color);
}

uint16_t DrawUtils::getStringWidthProp(const char *str, const ProportionalFont *font)
{
    uint16_t width = 0;

    while (*str != '\0')
    {
        uint8_t charIndex = *str - font->firstChar;
        width += font->widths[charIndex];
        str++;
    }

    return width;
}
