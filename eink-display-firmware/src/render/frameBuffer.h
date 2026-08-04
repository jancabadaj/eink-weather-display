#pragma once

#include <cstddef>
#include <cstdint>

#include "../config.h"

// Owns the display buffer (48 KB at 800x480)
class FrameBuffer
{
public:
    static constexpr size_t byteCount =
        static_cast<size_t>(Config::Display::widthBytes) * Config::Display::heightBytes;

    uint8_t *data() { return _data; }
    const uint8_t *data() const { return _data; }

    static constexpr size_t size() { return byteCount; }

private:
    uint8_t _data[byteCount] = {};
};
