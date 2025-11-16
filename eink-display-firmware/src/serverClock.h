#pragma once

#include <chrono>

class ServerClock
{
public:
    ServerClock() = default;

    // Sync the clock with server time
    // @param syncTimeMillis - The millis() value when the refresh was attempted
    // @param serverTime - The server's UTC timestamp
    void syncTime(unsigned long syncTimeMillis, std::chrono::milliseconds serverTime);

    // Get current UTC time in milliseconds since epoch
    // Returns calculated UTC timestamp based on last sync, or millis() if never synced
    unsigned long long getUtcTime() const;

private:
    unsigned long _lastSyncMillis = 0;
    unsigned long long _serverTimeAtSync;
    bool _hasSynced = false;
};
