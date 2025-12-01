#pragma once

#include <chrono>
#include <array>
#include <algorithm>
#include <memory>
#include "serverClock.h"

namespace UpdateSchedule
{
    // Define night hours (hour >= NIGHT_START_HOUR OR hour < NIGHT_END_HOUR is night)
    constexpr int NIGHT_START_HOUR_UTC = 21;
    constexpr int NIGHT_END_HOUR_UTC = 6;

    // Netatmo is refreshing data in documented intervals
    constexpr unsigned long REFRESH_INTERVAL_MS = 10 * 60 * 1000; // 10 minutes
    constexpr unsigned long INTERVAL_OFFSET_MS = 30 * 1000;         // 30 seconds offset to avoid fetching too early
    // Minimum interval to prevent rapid retries on errors, or too frequent updates if data was not updated for some reason
    constexpr unsigned long MIN_INTERVAL_MS = 2 * 60 * 1000; // 2 minutes
    // Maximum consecutive failures before stopping updates and clearing display
    constexpr int MAX_CONSECUTIVE_FAILURES = 3;

} // namespace UpdateSchedule

class UpdateScheduler
{
public:
    UpdateScheduler(std::shared_ptr<ServerClock> serverClock)
        : _serverClock(serverClock) {}

    // Schedule next refresh on data timestamp + refresh interval. Return false if night time.
    bool scheduleNormalRefresh(unsigned long long dataUtcTimestampMs);

    // In case of failure, schedule a retry after minimum interval. Return false if too many failures.
    bool scheduleRetryRefresh();

    unsigned long getNextScheduledRefreshMillis() const { return _nextRefreshMillis; }

private:
    static int getCurrentHour(unsigned long long currentUtcTimestampMs);
    static bool isNightTime(int hour);
    void setNextRefresh(unsigned long delayMs);

    std::shared_ptr<ServerClock> _serverClock;
    unsigned long _nextRefreshMillis = 0;
    int _consecutiveRetries = 0;
};
