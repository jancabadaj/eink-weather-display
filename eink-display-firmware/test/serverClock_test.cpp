#include <doctest.h>

#include "fakes/fakeClock.h"
#include "schedule/serverClock.h"

namespace
{
    // 2025-01-15 14:30:00 UTC
    constexpr uint64_t serverUtcMs = 1736951400000ULL;
} // namespace

TEST_CASE("ServerClock: fallsBackToUptimeBeforeFirstSync")
{
    FakeClock clock;
    clock.setUptimeMs(12345);
    ServerClock serverClock(clock);

    CHECK(serverClock.getUtcTime() == 12345);
}

TEST_CASE("ServerClock: returnsServerTimeImmediatelyAfterSync")
{
    FakeClock clock;
    clock.setUptimeMs(5000);
    ServerClock serverClock(clock);

    serverClock.syncTime(clock.uptimeMs(), std::chrono::milliseconds(serverUtcMs));

    CHECK(serverClock.getUtcTime() == serverUtcMs);
}

TEST_CASE("ServerClock: extrapolatesWithElapsedUptime")
{
    FakeClock clock;
    clock.setUptimeMs(5000);
    ServerClock serverClock(clock);
    serverClock.syncTime(clock.uptimeMs(), std::chrono::milliseconds(serverUtcMs));

    clock.advanceMs(9000);

    CHECK(serverClock.getUtcTime() == serverUtcMs + 9000);
}

TEST_CASE("ServerClock: resyncReplacesTheOffset")
{
    FakeClock clock;
    ServerClock serverClock(clock);
    serverClock.syncTime(clock.uptimeMs(), std::chrono::milliseconds(serverUtcMs));

    clock.advanceMs(60 * 60 * 1000); // an hour of drift

    // Server says something different; the new sync wins outright.
    const uint64_t corrected = serverUtcMs + 60 * 60 * 1000 + 4321;
    serverClock.syncTime(clock.uptimeMs(), std::chrono::milliseconds(corrected));

    CHECK(serverClock.getUtcTime() == corrected);
}
