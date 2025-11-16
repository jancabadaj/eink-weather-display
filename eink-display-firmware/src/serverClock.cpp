#include <Arduino.h>
#include "serverClock.h"
#include "logger.h"

void ServerClock::syncTime(unsigned long syncTimeMillis, std::chrono::milliseconds serverTime)
{
    _lastSyncMillis = syncTimeMillis;
    _serverTimeAtSync = serverTime.count();
    _hasSynced = true;
    logger.debug("[ServerClock] Syncing time. Server time: %llu, Sync millis: %lu",
                 _serverTimeAtSync,
                 syncTimeMillis);
}

unsigned long long ServerClock::getUtcTime() const
{
    if (!_hasSynced)
    {
        logger.warning("[ServerClock] getUtcTime called but clock has not been synced yet. Returning millis(): %lu", millis());
        return millis();
    }

    unsigned long elapsedSinceSync = millis() - _lastSyncMillis;
    unsigned long long currentUtcMs = _serverTimeAtSync + elapsedSinceSync;
    logger.debug("[ServerClock] Current UTC time calculated: %llu (server time: %llu, elapsed since sync: %lu)",
                 currentUtcMs, _serverTimeAtSync, elapsedSinceSync);
    return currentUtcMs;
}
