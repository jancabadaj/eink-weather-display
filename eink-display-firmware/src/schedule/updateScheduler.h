#pragma once

#include "../platform/clock.h"
#include "serverClock.h"
#include "../settings/configOverrides.h"
#include "../config.h"

class UpdateScheduler
{
public:
    UpdateScheduler(Clock &clock, ServerClock &serverClock, ConfigOverrides &configOverrides)
        : _clock(clock), _serverClock(serverClock), _configOverrides(configOverrides) {}

    // Schedule next refresh on data timestamp + refresh interval. Return false if night time.
    bool scheduleRefresh(uint64_t dataUtcTimestampMs);

    // Schedule next refresh on refresh interval (used after failure when there is no valid data timestamp)
    void scheduleRetry();

    uint64_t getNextScheduledRefreshMillis() const { return _nextRefreshMillis; }

private:
    void applyRateLimit();
    static int getCurrentHour(uint64_t currentUtcTimestampMs);
    static bool isNightTime(int hour, int nightStart, int nightEnd);

    Clock &_clock;
    ServerClock &_serverClock;
    ConfigOverrides &_configOverrides;
    uint64_t _nextRefreshMillis = 0;
    uint64_t _callTimestamps[Config::Schedule::maxCallsPerInterval] = {};
    int _callTimestampIndex = 0;
};
