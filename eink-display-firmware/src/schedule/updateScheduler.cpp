#include "updateScheduler.h"
#include "../logger.h"
#include "../config.h"

bool UpdateScheduler::scheduleRefresh(uint64_t dataUtcTimestampMs)
{
    const uint64_t currentUtcTimestampMs = _serverClock.getUtcTime();
    logger.info("[UpdateScheduler] Calculating next refresh delay. Current UTC: %llu, Data timestamp: %llu, Age: %llus",
                currentUtcTimestampMs, dataUtcTimestampMs,
                (currentUtcTimestampMs - dataUtcTimestampMs) / 1000);

    int currentHour = getCurrentHour(currentUtcTimestampMs);
    int nightStart = _configOverrides.getNightStartHour();
    int nightEnd = _configOverrides.getNightEndHour();
    bool isNight = isNightTime(currentHour, nightStart, nightEnd);

    if (isNight)
    {
        // Calculate when night ends
        // 1. Timestamp of current hour (0 minutes, 0 seconds)
        const time_t currentTimeSec = (time_t)(currentUtcTimestampMs / 1000);
        time_t currentHourTimestamp = currentTimeSec - (currentTimeSec % 3600);

        // 2. Calculate hours until night end
        int hoursUntilNightEnd = nightEnd - currentHour;
        if (hoursUntilNightEnd <= 0)
        {
            hoursUntilNightEnd += 24; // Night end is tomorrow
        }

        // 3. Timestamp of night end hour (and 0 minutes, 0 seconds)
        time_t nightEndTimestamp = currentHourTimestamp + (hoursUntilNightEnd * 3600);

        const uint64_t nextRefreshDelay = (uint64_t)(nightEndTimestamp - currentTimeSec) * 1000;
        _nextRefreshMillis = _clock.uptimeMs() + nextRefreshDelay;
        logger.info("[UpdateScheduler] Night mode - no updates until %d UTC (current: %d UTC, %llus remaining)",
                    nightEnd, currentHour, nextRefreshDelay / 1000);

        applyRateLimit();
        return false;
    }
    else
    {
        uint64_t expectedNextDataTimeMs = dataUtcTimestampMs + Config::Schedule::refreshIntervalMs + Config::Schedule::intervalOffsetMs;

        uint64_t nextRefreshDelay;
        while (expectedNextDataTimeMs <= currentUtcTimestampMs) // Handle stale data - add as many intervals as needed to get next expected refresh time in the future
        {
            expectedNextDataTimeMs += Config::Schedule::refreshIntervalMs;
        }
        nextRefreshDelay = expectedNextDataTimeMs - currentUtcTimestampMs;

        _nextRefreshMillis = _clock.uptimeMs() + nextRefreshDelay;
        logger.info("[UpdateScheduler] Next refresh scheduled in %llu seconds", nextRefreshDelay / 1000);

        applyRateLimit();
        return true;
    }
}

int UpdateScheduler::getCurrentHour(uint64_t currentUtcTimestampMs)
{
    const time_t currentTimeSec = (time_t)(currentUtcTimestampMs / 1000);

    struct tm timeinfo;
    gmtime_r(&currentTimeSec, &timeinfo);

    return timeinfo.tm_hour;
}

void UpdateScheduler::applyRateLimit()
{
    const uint64_t now = _clock.uptimeMs();

    // Sliding window - keep the last maxCallsPerInterval call timestamps in a ring buffer.
    const uint64_t oldest = _callTimestamps[_callTimestampIndex];
    _callTimestamps[_callTimestampIndex] = now;
    _callTimestampIndex = (_callTimestampIndex + 1) % Config::Schedule::maxCallsPerInterval;

    const uint64_t throttledMs = now + Config::Schedule::refreshIntervalMs;
    // If the oldest entry is still within the window, all calls happened within window
    if (oldest != 0 && now - oldest < Config::Schedule::refreshIntervalMs && _nextRefreshMillis < throttledMs)
    {
        logger.warning("[UpdateScheduler] Rate limit: %d calls within %lus window, throttling next refresh",
                       Config::Schedule::maxCallsPerInterval, Config::Schedule::refreshIntervalMs / 1000);
        _nextRefreshMillis = throttledMs;
    }
}

void UpdateScheduler::scheduleRetry()
{
    _nextRefreshMillis = _clock.uptimeMs() + Config::Schedule::refreshIntervalMs;
    logger.info("[UpdateScheduler] Retry scheduled in %lu seconds", Config::Schedule::refreshIntervalMs / 1000);
}

bool UpdateScheduler::isNightTime(int hour, int nightStart, int nightEnd)
{
    if (nightStart < nightEnd)
    {
        return hour >= nightStart && hour < nightEnd;
    }
    return hour >= nightStart || hour < nightEnd;
}
