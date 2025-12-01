#include <Arduino.h>
#include <memory>
#include "updateScheduler.h"
#include "logger.h"

bool UpdateScheduler::scheduleNormalRefresh(unsigned long long dataUtcTimestampMs)
{
    _consecutiveRetries = 0;

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
        setNextRefresh(nextRefreshDelay);
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
        if (expectedNextDataTimeMs <= currentUtcTimestampMs)
        {
            nextRefreshDelay = UpdateSchedule::MIN_INTERVAL_MS;
            logger.warning("[UpdateScheduler] Data is stale, next refresh would be in the past. Scheduling minimum interval.");
        }
        else
        {
            nextRefreshDelay = (unsigned long)(expectedNextDataTimeMs - currentUtcTimestampMs);
        }

        setNextRefresh(nextRefreshDelay);
        return true;
    }
}

bool UpdateScheduler::scheduleRetryRefresh()
{
    _consecutiveRetries++;

    logger.warning("[UpdateScheduler] Retry schedule %d/%d",
                   _consecutiveRetries, UpdateSchedule::MAX_CONSECUTIVE_FAILURES);

    if (_consecutiveRetries >= UpdateSchedule::MAX_CONSECUTIVE_FAILURES)
    {
        logger.error("[UpdateScheduler] Max consecutive schedule retries reached!");
        _nextRefreshMillis = ULONG_MAX;
        return false;
    }
    else
    {
        setNextRefresh(UpdateSchedule::MIN_INTERVAL_MS);
        return true;
    }
}

void UpdateScheduler::setNextRefresh(unsigned long delayMs)
{
    if (delayMs < UpdateSchedule::MIN_INTERVAL_MS)
    {
        delayMs = UpdateSchedule::MIN_INTERVAL_MS;
    }
    _nextRefreshMillis = millis() + delayMs;
    logger.info("[UpdateScheduler] Next refresh scheduled in %lu seconds", delayMs / 1000);
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