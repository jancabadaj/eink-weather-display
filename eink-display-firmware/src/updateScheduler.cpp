#include <Arduino.h>
#include <memory>
#include "updateScheduler.h"
#include "logger.h"
#include "config.h"

bool UpdateScheduler::scheduleRefresh(unsigned long long dataUtcTimestampMs)
{
    unsigned long long currentUtcTimestampMs = _serverClock->getUtcTime();
    logger.info("[UpdateScheduler] Calculating next refresh delay. Current UTC: %llu, Data timestamp: %llu, Age: %llus",
                currentUtcTimestampMs, dataUtcTimestampMs,
                (currentUtcTimestampMs - dataUtcTimestampMs) / 1000);

    int currentHour = getCurrentHour(currentUtcTimestampMs);
    int nightStart = _configOverrides->getNightStartHour();
    int nightEnd = _configOverrides->getNightEndHour();
    bool isNight = isNightTime(currentHour, nightStart, nightEnd);

    if (isNight)
    {
        // Calculate when night ends
        // 1. Timestamp of current hour (0 minutes, 0 seconds)
        time_t currentTimeSec = currentUtcTimestampMs / 1000;
        time_t currentHourTimestamp = currentTimeSec - (currentTimeSec % 3600);

        // 2. Calculate hours until night end
        int hoursUntilNightEnd = nightEnd - currentHour;
        if (hoursUntilNightEnd <= 0)
        {
            hoursUntilNightEnd += 24; // Night end is tomorrow
        }

        // 3. Timestamp of night end hour (and 0 minutes, 0 seconds)
        time_t nightEndTimestamp = currentHourTimestamp + (hoursUntilNightEnd * 3600);

        unsigned long nextRefreshDelay = (nightEndTimestamp - currentTimeSec) * 1000;
        _nextRefreshMillis = millis() + nextRefreshDelay;
        logger.info("[UpdateScheduler] Night mode - no updates until %d UTC (current: %d UTC, %lus remaining)",
                    nightEnd, currentHour, nextRefreshDelay / 1000);

        applyRateLimit();
        return false;
    }
    else
    {
        unsigned long long expectedNextDataTimeMs = dataUtcTimestampMs + Config::Schedule::refreshIntervalMs + Config::Schedule::intervalOffsetMs;

        unsigned long nextRefreshDelay;
        while (expectedNextDataTimeMs <= currentUtcTimestampMs) // Handle stale data - add as many intervals as needed to get next expected refresh time in the future
        {
            expectedNextDataTimeMs += Config::Schedule::refreshIntervalMs;
        }
        nextRefreshDelay = (unsigned long)(expectedNextDataTimeMs - currentUtcTimestampMs);

        _nextRefreshMillis = millis() + nextRefreshDelay;
        logger.info("[UpdateScheduler] Next refresh scheduled in %lu seconds", nextRefreshDelay / 1000);

        applyRateLimit();
        return true;
    }
}

int UpdateScheduler::getCurrentHour(unsigned long long currentUtcTimestampMs)
{
    time_t currentTimeSec = currentUtcTimestampMs / 1000;

    struct tm timeinfo;
    gmtime_r(&currentTimeSec, &timeinfo);

    return timeinfo.tm_hour;
}

void UpdateScheduler::applyRateLimit()
{
    unsigned long now = millis();

    // Sliding window - keep the last maxCallsPerInterval call timestamps in a ring buffer.
    unsigned long oldest = _callTimestamps[_callTimestampIndex];
    _callTimestamps[_callTimestampIndex] = now;
    _callTimestampIndex = (_callTimestampIndex + 1) % Config::Schedule::maxCallsPerInterval;

    unsigned long throttledMs = now + Config::Schedule::refreshIntervalMs;
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
    _nextRefreshMillis = millis() + Config::Schedule::refreshIntervalMs;
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
