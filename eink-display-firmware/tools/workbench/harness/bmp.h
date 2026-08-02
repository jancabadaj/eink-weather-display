#pragma once

// Minimal 1-bit BMP writer for the workbench preview.
//
// Lives in the harness, not in src/: the application does not need it until the
// device serves /display.bmp itself. Keeping it here means the tool adds nothing
// to the firmware.
//
// The frame buffer is already 1bpp, MSB first, bit set = white. A 1-bit BMP
// indexes a 2-colour palette, so palette[0]=black / palette[1]=white maps the
// bits straight through with no inversion. At 800px a row is exactly 100 bytes,
// already 4-byte aligned, so no row padding is needed either.

#include <cstdint>
#include <cstdio>
#include <vector>

namespace bmp
{
    inline void put16(std::vector<uint8_t> &v, uint16_t x)
    {
        v.push_back(x & 0xFF);
        v.push_back((x >> 8) & 0xFF);
    }

    inline void put32(std::vector<uint8_t> &v, uint32_t x)
    {
        v.push_back(x & 0xFF);
        v.push_back((x >> 8) & 0xFF);
        v.push_back((x >> 16) & 0xFF);
        v.push_back((x >> 24) & 0xFF);
    }

    inline bool write1Bit(const char *path, const uint8_t *frame, int width, int height)
    {
        const int rowBytes = width / 8;
        const uint32_t dataOffset = 14 + 40 + 8; // header + info + 2-colour palette
        const uint32_t imageSize = (uint32_t)rowBytes * height;

        std::vector<uint8_t> out;
        out.reserve(dataOffset + imageSize);

        out.push_back('B');
        out.push_back('M');
        put32(out, dataOffset + imageSize);
        put32(out, 0);
        put32(out, dataOffset);

        put32(out, 40);
        put32(out, (uint32_t)width);
        put32(out, (uint32_t)height);
        put16(out, 1);
        put16(out, 1);
        put32(out, 0);
        put32(out, imageSize);
        put32(out, 2835);
        put32(out, 2835);
        put32(out, 2);
        put32(out, 2);

        // Palette: index 0 = black, index 1 = white (BGRA)
        put32(out, 0x00000000);
        put32(out, 0x00FFFFFF);

        // BMP rows run bottom-up
        for (int y = height - 1; y >= 0; y--)
        {
            const uint8_t *row = frame + (size_t)y * rowBytes;
            out.insert(out.end(), row, row + rowBytes);
        }

        FILE *f = fopen(path, "wb");
        if (!f)
        {
            return false;
        }
        const size_t written = fwrite(out.data(), 1, out.size(), f);
        fclose(f);
        return written == out.size();
    }
} // namespace bmp
