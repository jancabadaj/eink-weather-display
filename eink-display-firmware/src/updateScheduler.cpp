#include <Arduino.h>
#include <memory>
#include "updateScheduler.h"
#include "logger.h"

void UpdateScheduler::addIntervalSample(unsigned long intervalMs)
{
    if (intervalMs < UpdateSchedule::MIN_VALID_INTERVAL_MS ||
        intervalMs > UpdateSchedule::MAX_VALID_INTERVAL_MS)
    {
        logger.warning("[UpdateScheduler] Ignoring invalid sample: %lus (expected %lu-%lus)",
                       intervalMs / 1000,
                       UpdateSchedule::MIN_VALID_INTERVAL_MS / 1000,
                       UpdateSchedule::MAX_VALID_INTERVAL_MS / 1000);
        return;
    }

    _intervalSamples[_sampleIndex] = intervalMs;
    _sampleIndex = (_sampleIndex + 1) % UpdateSchedule::INTERVAL_SAMPLE_SIZE;

    if (_sampleCount < UpdateSchedule::INTERVAL_SAMPLE_SIZE)
    {
        _sampleCount++;
    }

    logger.debug("[UpdateScheduler] Sample added: %lus (%u/%u)",
                 intervalMs / 1000,
                 _sampleCount,
                 UpdateSchedule::INTERVAL_SAMPLE_SIZE);
}

unsigned long UpdateScheduler::getMedianInterval() const
{
    if (_sampleCount == 0)
    {
        return 0;
    }

    std::array<unsigned long, UpdateSchedule::INTERVAL_SAMPLE_SIZE> sortedSamples;
    for (size_t i = 0; i < _sampleCount; i++)
    {
        sortedSamples[i] = _intervalSamples[i];
    }
    std::sort(sortedSamples.begin(), sortedSamples.begin() + _sampleCount);

    // Average middle values if even number, else take middle value
    size_t medianIndex = _sampleCount / 2;
    if (_sampleCount % 2 == 0 && _sampleCount > 1)
    {
        return (sortedSamples[medianIndex - 1] + sortedSamples[medianIndex]) / 2;
    }
    return sortedSamples[medianIndex];
}

bool UpdateScheduler::scheduleNextRefresh(unsigned long long dataUtcTimestampMs)
{
    unsigned long long currentUtcTimestampMs = _serverClock->getUtcTime();
    logger.info("[UpdateScheduler] Calculating next refresh delay. Current UTC time: %llu, Data timestamp: %llu",
                currentUtcTimestampMs, dataUtcTimestampMs);
    int currentHour = getCurrentHour(currentUtcTimestampMs);
    bool isNight = isNightTime(currentHour);

    unsigned long nextRefreshDelay;

    if (isNight)
    {
        // Calculate exact timestamp when night ends
        time_t currentTimeSec = currentUtcTimestampMs / 1000;
        struct tm timeinfo;
        gmtime_r(&currentTimeSec, &timeinfo);

        // Set to night end hour with 0 minutes and seconds
        timeinfo.tm_hour = UpdateSchedule::NIGHT_END_HOUR_UTC;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = 0;

        time_t nightEndTimestamp = mktime(&timeinfo);

        // If night end is before current time, it's tomorrow
        if (nightEndTimestamp <= currentTimeSec)
        {
            timeinfo.tm_mday += 1;
            nightEndTimestamp = mktime(&timeinfo);
        }

        nextRefreshDelay = (nightEndTimestamp - currentTimeSec) * 1000;

        logger.info("[UpdateScheduler] Night mode - no updates until %d UTC (current: %d UTC, %lus remaining)",
                    UpdateSchedule::NIGHT_END_HOUR_UTC,
                    currentHour,
                    nextRefreshDelay / 1000);
        setNextScheduledRefreshMillis(millis() + nextRefreshDelay);
        return false;
    }
    else
    {
        unsigned long medianInterval = getMedianInterval();

        if (medianInterval > 0)
        {
            unsigned long expectedNextDataTimeMs = dataUtcTimestampMs + medianInterval + UpdateSchedule::ADAPTIVE_UPDATE_OFFSET_MS;
            long delayFromNow = (long)(expectedNextDataTimeMs - currentUtcTimestampMs);

            if (delayFromNow < (long)UpdateSchedule::MIN_REFRESH_INTERVAL_MS)
            {
                nextRefreshDelay = UpdateSchedule::MIN_REFRESH_INTERVAL_MS;
                logger.warning("[UpdateScheduler] Data overdue - using minimum delay: %lus",
                               nextRefreshDelay / 1000);
            }
            else
            {
                nextRefreshDelay = (unsigned long)delayFromNow;
            }

            logger.info("[UpdateScheduler] Adaptive schedule: %lus (median: %lus, n=%u, data age: %lus)",
                        nextRefreshDelay / 1000,
                        medianInterval / 1000,
                        _sampleCount,
                        (currentUtcTimestampMs - dataUtcTimestampMs) / 1000);
        }
        else
        {
            nextRefreshDelay = UpdateSchedule::DEFAULT_UPDATE_INTERVAL_MS;
            logger.info("[UpdateScheduler] Default schedule: %lus (learning...)",
                        nextRefreshDelay / 1000);
        }
    }

    logger.info("[UpdateScheduler] Next refresh in: %lus", nextRefreshDelay / 1000);
    setNextScheduledRefreshMillis(millis() + nextRefreshDelay);
    return true;
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