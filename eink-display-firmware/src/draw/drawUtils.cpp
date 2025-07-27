#include "drawUtils.h"
#include "../definitions.h"

#include "fonts.h"

#define COLOR_WHITE 0xFF
#define COLOR_BLACK 0x00

#define pgmc_read_byte(addr) (*(const unsigned char *)(addr))
#define pgmc_read_word(addr) ({       \
    typeof(addr) _addr = (addr);      \
    *(const unsigned short *)(_addr); \
})

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

void DrawUtils::drawString(uint8_t *imageData, uint16_t x, uint16_t y, const char *str, const Font *font, Color color)
{
    if (x > IMAGE_WIDTH || y > IMAGE_HEIGHT)
    {
        return;
    }

    if ((y + font->height) > IMAGE_HEIGHT)
    {
        return; // String exceeds image height
    }

    uint16_t xx = x;

    while (*str != '\0')
    {
        if ((xx + font->width) > IMAGE_WIDTH)
        {
            return; // String exceeds image width
        }

        drawChar(imageData, xx, y, *str, font, color);

        // The next character of the address
        str++;

        xx += font->width;
    }
}

void DrawUtils::drawChar(uint8_t *imageData, uint16_t x, uint16_t y, const char c, const Font *font, Color color)
{
    if (x > IMAGE_WIDTH || y > IMAGE_HEIGHT)
    {
        return;
    }

    uint32_t charOffset = (c - ' ') * font->height * (font->width / 8 + (font->width % 8 ? 1 : 0));
    const unsigned char *ptr = &font->table[charOffset];

    uint16_t row, col;
    for (row = 0; row < font->height; row++)
    {
        for (col = 0; col < font->width; col++)
        {
            if (*ptr & (0x80 >> (col % 8)))
                setPixel(imageData, x + col, y + row, color);

            if (col % 8 == 7)
                ptr++;
        }

        if (font->width % 8 != 0)
            ptr++;
    }
}
