#pragma once

#include <chrono>
#include <array>
#include <algorithm>
#include <memory>
#include "serverClock.h"

namespace UpdateSchedule
{
    // ============= CONFIGURABLE UPDATE SCHEDULE
    // Define night hours (hour >= NIGHT_START_HOUR OR hour < NIGHT_END_HOUR is night)
    constexpr int NIGHT_START_HOUR_UTC = 21;
    constexpr int NIGHT_END_HOUR_UTC = 6;

    // ============= ADAPTIVE LEARNING PARAMETERS
    // Number of interval samples to track for calculating median
    constexpr size_t INTERVAL_SAMPLE_SIZE = 7;
    // Valid interval range (ignore samples outside this range as anomalies)
    constexpr unsigned long MIN_VALID_INTERVAL_MS = 5 * 60 * 1000;       // 5 minutes
    constexpr unsigned long MAX_VALID_INTERVAL_MS = 20 * 60 * 1000;      // 20 minutes
    constexpr unsigned long DEFAULT_UPDATE_INTERVAL_MS = 10 * 60 * 1000; // 10 minutes - used if not learned yet
    // Refresh after predicted update time plus offset
    constexpr unsigned long ADAPTIVE_UPDATE_OFFSET_MS = 30 * 1000; // 30 seconds after predicted update

    // ============= RETRY AND ERROR HANDLING
    // Minimum interval between refresh attempts (prevent rapid retries on errors)
    constexpr unsigned long MIN_REFRESH_INTERVAL_MS = 2 * 60 * 1000; // 2 minutes
    // Maximum consecutive failures before stopping updates and clearing display
    constexpr int MAX_CONSECUTIVE_FAILURES = 3;

} // namespace UpdateSchedule

class UpdateScheduler
{
public:
    UpdateScheduler(std::shared_ptr<ServerClock> serverClock)
        : _serverClock(serverClock) {}

    void addIntervalSample(unsigned long intervalMs);
    // Schedule next refresh based on data timestamp. Return false if night time.
    bool scheduleNextRefresh(unsigned long dataUtcTimestampMs);

    unsigned long getNextScheduledRefreshMillis() const { return _nextScheduledRefreshMillis; }
    void setNextScheduledRefreshMillis(unsigned long nextRefreshMillis) { _nextScheduledRefreshMillis = nextRefreshMillis; }

private:
    unsigned long getMedianInterval() const;
    static int getCurrentHour(unsigned long currentUtcTimestampMs);
    static bool isNightTime(int hour);

    std::shared_ptr<ServerClock> _serverClock;
    std::array<unsigned long, UpdateSchedule::INTERVAL_SAMPLE_SIZE> _intervalSamples = {0};
    size_t _sampleCount = 0;
    size_t _sampleIndex = 0;
    unsigned long _nextScheduledRefreshMillis = 0;
};
