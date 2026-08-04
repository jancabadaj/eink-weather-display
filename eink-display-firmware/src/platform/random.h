#pragma once

#include <cstdint>

class Random
{
public:
    virtual ~Random() = default;
    virtual uint32_t next() = 0;
};
