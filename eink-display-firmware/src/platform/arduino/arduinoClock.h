#pragma once

#include <Arduino.h>

#include "../clock.h"

class ArduinoClock : public Clock
{
public:
    uint64_t uptimeMs() const override
    {
        const uint32_t raw = millis();
        if (raw < _lastRaw)
        {
            _wraps++;
        }
        _lastRaw = raw;
        return (static_cast<uint64_t>(_wraps) << 32) | raw;
    }

private:
    mutable uint32_t _lastRaw = 0;
    mutable uint32_t _wraps = 0;
};
