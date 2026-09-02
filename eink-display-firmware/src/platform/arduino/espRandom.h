#pragma once

#include "../random.h"

// esp_random() draws from the SoC hardware RNG, which is a true random source
class EspRandom : public Random
{
public:
    uint32_t next() override;
};
