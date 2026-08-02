#pragma once

#include <memory>
#include "serverClock.h"
#include "../settings/configOverrides.h"
#include "../config.h"

class UpdateScheduler
{
public:
    UpdateScheduler(std::shared_ptr<ServerClock> serverClock,
                    std::shared_ptr<ConfigOverrides> configOverrides)
        : _serverClock(serverClock), _configOverrides(configOverrides) {}

    // Schedule next refresh on data timestamp + refresh interval. Return false if night time.
    bool scheduleRefresh(unsigned long long dataUtcTimestampMs);

    // Schedule next refresh on refresh interval (used after failure when there is no valid data timestamp)
    void scheduleRetry();

    unsigned long getNextScheduledRefreshMillis() const { return _nextRefreshMillis; }

private:
    void applyRateLimit();
    static int getCurrentHour(unsigned long long currentUtcTimestampMs);
    static bool isNightTime(int hour, int nightStart, int nightEnd);

    std::shared_ptr<ServerClock> _serverClock;
    std::shared_ptr<ConfigOverrides> _configOverrides;
    unsigned long _nextRefreshMillis = 0;
    unsigned long _callTimestamps[Config::Schedule::maxCallsPerInterval] = {};
    int _callTimestampIndex = 0;
};
