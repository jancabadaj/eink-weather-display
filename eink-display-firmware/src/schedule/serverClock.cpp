#include "serverClock.h"
#include "../logger.h"

void ServerClock::syncTime(uint64_t syncUptimeMs, std::chrono::milliseconds serverTime)
{
    _lastSyncUptimeMs = syncUptimeMs;
    _serverTimeAtSync = serverTime.count();
    _hasSynced = true;
    logger.debug("[ServerClock] Syncing time. Server time: %llu, Sync uptime: %llu",
                 _serverTimeAtSync,
                 syncUptimeMs);
}

uint64_t ServerClock::getUtcTime() const
{
    if (!_hasSynced)
    {
        logger.warning("[ServerClock] getUtcTime called but clock has not been synced yet. Returning uptime: %llu",
                       _clock.uptimeMs());
        return _clock.uptimeMs();
    }

    const uint64_t elapsedSinceSync = _clock.uptimeMs() - _lastSyncUptimeMs;
    const uint64_t currentUtcMs = _serverTimeAtSync + elapsedSinceSync;
    logger.debug("[ServerClock] Current UTC time calculated: %llu (server time: %llu, elapsed since sync: %llu)",
                 currentUtcMs, _serverTimeAtSync, elapsedSinceSync);
    return currentUtcMs;
}
