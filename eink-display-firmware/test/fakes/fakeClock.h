#pragma once

#include "platform/clock.h"

class FakeClock : public Clock
{
public:
    uint64_t uptimeMs() const override { return _now; }

    void advanceMs(uint64_t ms) { _now += ms; }
    void setUptimeMs(uint64_t ms) { _now = ms; }

private:
    uint64_t _now = 0;
};
