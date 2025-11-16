#include <Arduino.h>
#include "serverClock.h"
#include "logger.h"

void ServerClock::syncTime(unsigned long syncTimeMillis, std::chrono::system_clock::time_point serverTime)
{
    _lastSyncMillis = syncTimeMillis;
    _serverTimeAtSync = serverTime;
    _hasSynced = true;
}

unsigned long ServerClock::getUtcTime() const
{
    if (!_hasSynced)
    {
        logger.warning("[ServerClock] getUtcTime called but clock has not been synced yet. Returning millis(): %lu", millis());
        return millis();
    }

    auto serverTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        _serverTimeAtSync.time_since_epoch());

    unsigned long elapsedSinceSync = millis() - _lastSyncMillis;
    unsigned long currentUtcMs = serverTimeMs.count() + elapsedSinceSync;
    logger.debug("[ServerClock] Current UTC time calculated: %lu (server time: %lu, elapsed since sync: %lu)",
                 currentUtcMs, serverTimeMs.count(), elapsedSinceSync);
    return currentUtcMs;
}
