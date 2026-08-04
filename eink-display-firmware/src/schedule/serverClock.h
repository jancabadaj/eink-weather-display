#pragma once

#include <chrono>
#include <cstdint>

#include "../platform/clock.h"

// Extrapolates UTC time based on the last known server time and the local uptime
class ServerClock
{
public:
    explicit ServerClock(Clock &clock) : _clock(clock) {}

    // @param syncUptimeMs uptime when the request that carried serverTime was sent
    // @param serverTime  the server's UTC timestamp
    void syncTime(uint64_t syncUptimeMs, std::chrono::milliseconds serverTime);

    // Current UTC in ms since epoch, or uptime if never synced.
    uint64_t getUtcTime() const;

private:
    Clock &_clock;
    uint64_t _lastSyncUptimeMs = 0;
    uint64_t _serverTimeAtSync = 0;
    bool _hasSynced = false;
};
