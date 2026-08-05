#pragma once

#include <cstdint>

#include "platform/random.h"

// Counts up, so generated values are predictable in assertions
class FakeRandom : public Random
{
public:
    uint32_t nextValue = 1;

    uint32_t next() override { return nextValue++; }
};
