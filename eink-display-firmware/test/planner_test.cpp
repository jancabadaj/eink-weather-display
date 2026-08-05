#include <doctest.h>

#include "schedule/planner.h"

namespace
{
    // 2025-01-15, on the hour, so offsets below read as wall-clock hours UTC.
    constexpr uint64_t midnight = 1736899200000ULL;

    uint64_t atHour(int hour, int minutes = 0)
    {
        return midnight + static_cast<uint64_t>(hour) * 3600000ULL + static_cast<uint64_t>(minutes) * 60000ULL;
    }

    Planner::Settings nightAt(int start, int end)
    {
        Planner::Settings s;
        s.nightStartHourUtc = start;
        s.nightEndHourUtc = end;
        return s;
    }
} // namespace

TEST_CASE("Planner: derivesHourOfDayFromUtc")
{
    CHECK(Planner::hourOfDayUtc(midnight) == 0);
    CHECK(Planner::hourOfDayUtc(atHour(13, 59)) == 13);
    CHECK(Planner::hourOfDayUtc(atHour(23, 59)) == 23);
}

TEST_CASE("Planner: nightWindowWrappingMidnight")
{
    // 21:00 -> 05:00 spans midnight
    CHECK(Planner::isNightTime(21, 21, 5));
    CHECK(Planner::isNightTime(23, 21, 5));
    CHECK(Planner::isNightTime(0, 21, 5));
    CHECK(Planner::isNightTime(4, 21, 5));

    CHECK_FALSE(Planner::isNightTime(5, 21, 5)); // end hour is already day
    CHECK_FALSE(Planner::isNightTime(12, 21, 5));
    CHECK_FALSE(Planner::isNightTime(20, 21, 5)); // start hour not yet night
}

TEST_CASE("Planner: nightWindowWithinOneDay")
{
    // 01:00 -> 05:00 does not wrap
    CHECK(Planner::isNightTime(1, 1, 5));
    CHECK(Planner::isNightTime(4, 1, 5));
    CHECK_FALSE(Planner::isNightTime(0, 1, 5));
    CHECK_FALSE(Planner::isNightTime(5, 1, 5));
    CHECK_FALSE(Planner::isNightTime(22, 1, 5));
}

TEST_CASE("Planner: schedulesJustAfterTheStationsNextPublish")
{
    Planner::Settings settings;
    const uint64_t dataAt = atHour(12);
    const uint64_t now = atHour(12, 1);

    const Planner::Plan plan = Planner::planNext(now, dataAt, settings);

    CHECK(plan.mode == Planner::Mode::Normal);
    // data + 10min interval + 30s offset, measured from now (1 min later)
    CHECK(plan.delayMs == settings.refreshIntervalMs + settings.intervalOffsetMs - 60000ULL);
}

TEST_CASE("Planner: skipsWholeIntervalsWhenDataIsStale")
{
    Planner::Settings settings;
    const uint64_t dataAt = atHour(12);
    const uint64_t now = atHour(12, 35); // three intervals have already passed

    const Planner::Plan plan = Planner::planNext(now, dataAt, settings);

    CHECK(plan.mode == Planner::Mode::Normal);
    CHECK(plan.delayMs > 0);
    CHECK(plan.delayMs <= settings.refreshIntervalMs);
}

TEST_CASE("Planner: neverSchedulesInThePast")
{
    Planner::Settings settings;
    const uint64_t dataAt = atHour(0);

    for (int minute = 0; minute < 24 * 60; minute += 7)
    {
        const uint64_t now = atHour(8) + static_cast<uint64_t>(minute) * 60000ULL;
        if (Planner::isNightTime(Planner::hourOfDayUtc(now), settings.nightStartHourUtc,
                                 settings.nightEndHourUtc))
        {
            continue;
        }
        CHECK(Planner::planNext(now, dataAt, settings).delayMs > 0);
    }
}

TEST_CASE("Planner: nightSleepEndsOnTheHourNightEnds")
{
    const Planner::Settings settings = nightAt(21, 5);
    const uint64_t now = atHour(23, 30);

    const Planner::Plan plan = Planner::planNext(now, atHour(23), settings);

    CHECK(plan.mode == Planner::Mode::Night);
    // 23:30 -> 05:00 is 5h30m
    CHECK(plan.delayMs == (5ULL * 3600 + 30 * 60) * 1000ULL);
}

TEST_CASE("Planner: nightSleepAfterMidnightWakesSameMorning")
{
    const Planner::Settings settings = nightAt(21, 5);
    const uint64_t now = atHour(2, 15);

    const Planner::Plan plan = Planner::planNext(now, atHour(2), settings);

    CHECK(plan.mode == Planner::Mode::Night);
    CHECK(plan.delayMs == (2ULL * 3600 + 45 * 60) * 1000ULL);
}

TEST_CASE("Planner: throttlePassesDelayThroughWhileWindowHasRoom")
{
    Planner::RateLimitState state;
    const uint64_t interval = 600000;

    CHECK(Planner::throttle(1000, 5000, interval, state) == 5000);
    CHECK(Planner::throttle(2000, 5000, interval, state) == 5000);
}

TEST_CASE("Planner: throttleStretchesDelayOnceWindowIsFull")
{
    Planner::RateLimitState state;
    const uint64_t interval = 600000;

    // Fill the window with rapid calls.
    for (size_t i = 0; i < Planner::RateLimitState::windowSize; i++)
    {
        Planner::throttle(1000 + i * 10, 5000, interval, state);
    }

    // The oldest entry is still inside the window, so the short delay is refused.
    CHECK(Planner::throttle(1500, 5000, interval, state) == interval);
}

TEST_CASE("Planner: throttleLeavesLongDelaysAlone")
{
    Planner::RateLimitState state;
    const uint64_t interval = 600000;

    for (size_t i = 0; i < Planner::RateLimitState::windowSize; i++)
    {
        Planner::throttle(1000 + i * 10, 5000, interval, state);
    }

    // A night-length delay already exceeds the interval and stays untouched.
    CHECK(Planner::throttle(1500, interval * 5, interval, state) == interval * 5);
}

TEST_CASE("Planner: throttleForgetsCallsOlderThanTheWindow")
{
    Planner::RateLimitState state;
    const uint64_t interval = 600000;

    for (size_t i = 0; i < Planner::RateLimitState::windowSize; i++)
    {
        Planner::throttle(1000 + i * 10, 5000, interval, state);
    }

    // Long after the window, the burst no longer counts.
    CHECK(Planner::throttle(1000 + interval * 2, 5000, interval, state) == 5000);
}
