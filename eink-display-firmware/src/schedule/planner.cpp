#include "planner.h"

namespace Planner
{
    int hourOfDayUtc(uint64_t utcMs)
    {
        return static_cast<int>((utcMs / 1000 / 3600) % 24);
    }

    bool isNightTime(int hour, int nightStartHour, int nightEndHour)
    {
        if (nightStartHour < nightEndHour)
        {
            return hour >= nightStartHour && hour < nightEndHour;
        }
        return hour >= nightStartHour || hour < nightEndHour;
    }

    Plan planNext(uint64_t nowUtcMs, uint64_t dataUtcMs, const Settings &settings)
    {
        const int currentHour = hourOfDayUtc(nowUtcMs);

        if (isNightTime(currentHour, settings.nightStartHourUtc, settings.nightEndHourUtc))
        {
            // Wake on the hour night ends
            const uint64_t nowSec = nowUtcMs / 1000;
            const uint64_t currentHourSec = nowSec - (nowSec % 3600);

            int hoursUntilNightEnd = settings.nightEndHourUtc - currentHour;
            if (hoursUntilNightEnd <= 0)
            {
                hoursUntilNightEnd += 24; // Night end is tomorrow
            }

            const uint64_t nightEndSec =
                currentHourSec + static_cast<uint64_t>(hoursUntilNightEnd) * 3600;
            return {(nightEndSec - nowSec) * 1000, Mode::Night};
        }

        // Aim just past the moment the station is expected to publish again, skipping whole intervals when the data is already stale
        uint64_t expectedNextDataMs = dataUtcMs + settings.refreshIntervalMs + settings.intervalOffsetMs;
        while (expectedNextDataMs <= nowUtcMs) // Keep adding intervals until the next one is in the future
        {
            expectedNextDataMs += settings.refreshIntervalMs;
        }

        return {expectedNextDataMs - nowUtcMs, Mode::Normal};
    }

    uint64_t throttle(uint64_t nowUptimeMs, uint64_t proposedDelayMs, uint64_t refreshIntervalMs,
                      RateLimitState &state)
    {
        const uint64_t oldest = state.callUptimesMs[state.index];
        state.callUptimesMs[state.index] = nowUptimeMs;
        state.index = (state.index + 1) % RateLimitState::windowSize;

        const bool windowFull = oldest != 0 && nowUptimeMs - oldest < refreshIntervalMs;
        if (windowFull && proposedDelayMs < refreshIntervalMs)
        {
            return refreshIntervalMs;
        }
        return proposedDelayMs;
    }
} // namespace Planner
