#pragma once

#include <cstdint>

class DisplayPanel
{
public:
    virtual ~DisplayPanel() = default;

    virtual void present(const uint8_t *frame) = 0;
    virtual void clear() = 0;
};
