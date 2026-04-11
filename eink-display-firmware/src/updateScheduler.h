#pragma once

#include <memory>
#include "serverClock.h"
#include "configOverrides.h"

class UpdateScheduler
{
public:
    UpdateScheduler(std::shared_ptr<ServerClock> serverClock,
                    std::shared_ptr<ConfigOverrides> configOverrides)
        : _serverClock(serverClock), _configOverrides(configOverrides) {}

    // Schedule next refresh on data timestamp + refresh interval. Return false if night time.
    bool scheduleRefresh(unsigned long long dataUtcTimestampMs);

    unsigned long getNextScheduledRefreshMillis() const { return _nextRefreshMillis; }

private:
    static int getCurrentHour(unsigned long long currentUtcTimestampMs);
    static bool isNightTime(int hour, int nightStart, int nightEnd);

    std::shared_ptr<ServerClock> _serverClock;
    std::shared_ptr<ConfigOverrides> _configOverrides;
    unsigned long _nextRefreshMillis = 0;
};
