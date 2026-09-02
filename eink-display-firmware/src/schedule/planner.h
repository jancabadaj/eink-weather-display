#pragma once

#include <cstddef>
#include <cstdint>

#include "../config.h"

namespace Planner
{
    enum class Mode
    {
        Normal,
        Night
    };

    struct Settings
    {
        int nightStartHourUtc = Config::Schedule::nightStartHourUtc;
        int nightEndHourUtc = Config::Schedule::nightEndHourUtc;
        uint64_t refreshIntervalMs = Config::Schedule::refreshIntervalMs;
        uint64_t intervalOffsetMs = Config::Schedule::intervalOffsetMs;
    };

    struct Plan
    {
        uint64_t delayMs = 0;
        Mode mode = Mode::Normal;
    };

    // Uptimes of the most recent scheduling decisions, oldest overwritten first
    struct RateLimitState
    {
        static constexpr size_t windowSize = Config::Schedule::maxCallsPerInterval;

        uint64_t callUptimesMs[windowSize] = {};
        size_t index = 0;
    };

    int hourOfDayUtc(uint64_t utcMs);
    bool isNightTime(int hour, int nightStartHour, int nightEndHour);

    // When to wake next, measured from nowUtcMs.
    Plan planNext(uint64_t nowUtcMs, uint64_t dataUtcMs, const Settings &settings);

    // Records plan in the window and stretches the delay when the window is already full
    // Prevents a server returning stale data making the device poll faster than one interval
    uint64_t throttle(uint64_t nowUptimeMs, uint64_t proposedDelayMs, uint64_t refreshIntervalMs,
                      RateLimitState &state);
} // namespace Planner
