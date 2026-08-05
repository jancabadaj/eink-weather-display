#pragma once

#include "../platform/clock.h"
#include "../settings/configOverrides.h"
#include "planner.h"
#include "serverClock.h"

// Holds the current refresh time and manages the scheduling rules
class UpdateScheduler
{
public:
    UpdateScheduler(Clock &clock, ServerClock &serverClock, ConfigOverrides &configOverrides)
        : _clock(clock), _serverClock(serverClock), _configOverrides(configOverrides) {}

    // Schedules the next refresh from the data timestamp and reports which mode the display should show until then
    Planner::Mode scheduleRefresh(uint64_t dataUtcTimestampMs);

    // Schedules a plain interval, for when a failure left no usable timestamp
    void scheduleRetry();

    uint64_t getNextScheduledRefreshMillis() const { return _nextRefreshMillis; }

private:
    Planner::Settings settings() const;

    Clock &_clock;
    ServerClock &_serverClock;
    ConfigOverrides &_configOverrides;

    uint64_t _nextRefreshMillis = 0;
    Planner::RateLimitState _rateLimit;
};
