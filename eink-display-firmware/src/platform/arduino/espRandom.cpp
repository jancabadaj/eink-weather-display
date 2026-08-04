#include "espRandom.h"

#include <esp_random.h>

uint32_t EspRandom::next()
{
    return esp_random();
}
