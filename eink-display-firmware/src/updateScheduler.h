#pragma once

#include <chrono>
#include <array>
#include <algorithm>
#include <memory>
#include "serverClock.h"


class UpdateScheduler
{
public:
    UpdateScheduler(std::shared_ptr<ServerClock> serverClock)
        : _serverClock(serverClock) {}

    // Schedule next refresh on data timestamp + refresh interval. Return false if night time.
    bool scheduleRefresh(unsigned long long dataUtcTimestampMs);

    unsigned long getNextScheduledRefreshMillis() const { return _nextRefreshMillis; }

private:
    static int getCurrentHour(unsigned long long currentUtcTimestampMs);
    static bool isNightTime(int hour);
    void setNextRefresh(unsigned long delayMs);

    std::shared_ptr<ServerClock> _serverClock;
    unsigned long _nextRefreshMillis = 0;
};
