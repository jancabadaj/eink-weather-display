#include <Arduino.h>
#include <memory>
#include "updateScheduler.h"
#include "logger.h"
#include "definitions.h"

bool UpdateScheduler::scheduleRefresh(unsigned long long dataUtcTimestampMs)
{

    unsigned long long currentUtcTimestampMs = _serverClock->getUtcTime();
    logger.info("[UpdateScheduler] Calculating next refresh delay. Current UTC time: %llu, Data timestamp: %llu, Age: %llus",
                currentUtcTimestampMs, dataUtcTimestampMs,
                (currentUtcTimestampMs - dataUtcTimestampMs) / 1000);
    int currentHour = getCurrentHour(currentUtcTimestampMs);
    bool isNight = isNightTime(currentHour);

    if (isNight)
    {
        // Calculate when night ends
        // 1. Timestamp of current hour (0 minutes, 0 seconds)
        time_t currentTimeSec = currentUtcTimestampMs / 1000;
        time_t currentHourTimestamp = currentTimeSec - (currentTimeSec % 3600);

        // 2. Calculate hours until night end
        int hoursUntilNightEnd = UpdateSchedule::NIGHT_END_HOUR_UTC - currentHour;
        if (hoursUntilNightEnd <= 0)
        {
            hoursUntilNightEnd += 24; // Night end is tomorrow
        }

        // 3. Timestamp of night end hour (and 0 minutes, 0 seconds)
        time_t nightEndTimestamp = currentHourTimestamp + (hoursUntilNightEnd * 3600);

        unsigned long nextRefreshDelay = (nightEndTimestamp - currentTimeSec) * 1000;
        _nextRefreshMillis = millis() + nextRefreshDelay;
        logger.info("[UpdateScheduler] Night mode - no updates until %d UTC (current: %d UTC, %lus remaining)",
                    UpdateSchedule::NIGHT_END_HOUR_UTC,
                    currentHour,
                    nextRefreshDelay / 1000);
        return false;
    }
    else
    {
        unsigned long long expectedNextDataTimeMs = dataUtcTimestampMs + UpdateSchedule::REFRESH_INTERVAL_MS + UpdateSchedule::INTERVAL_OFFSET_MS;

        unsigned long nextRefreshDelay;
        while (expectedNextDataTimeMs <= currentUtcTimestampMs) // Handle stale data - add as many intervals as needed to get next expected refresh time in the future
        {
            expectedNextDataTimeMs += UpdateSchedule::REFRESH_INTERVAL_MS;
        }
        nextRefreshDelay = (unsigned long)(expectedNextDataTimeMs - currentUtcTimestampMs);

        _nextRefreshMillis = millis() + nextRefreshDelay;
        logger.info("[UpdateScheduler] Next refresh scheduled in %lu seconds", nextRefreshDelay / 1000);
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

bool UpdateScheduler::isNightTime(int hour)
{
    if (UpdateSchedule::NIGHT_START_HOUR_UTC < UpdateSchedule::NIGHT_END_HOUR_UTC)
    {
        return hour >= UpdateSchedule::NIGHT_START_HOUR_UTC && hour < UpdateSchedule::NIGHT_END_HOUR_UTC;
    }
    return hour >= UpdateSchedule::NIGHT_START_HOUR_UTC || hour < UpdateSchedule::NIGHT_END_HOUR_UTC;
}