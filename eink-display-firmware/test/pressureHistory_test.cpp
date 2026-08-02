#include <doctest.h>

#include "domain/pressureHistory.h"

namespace
{
    // 48h over 24 bars -> one bar per 2h. Buckets are absolute (timestamp / size),
    // so boundaries are aligned to the epoch, not to the first reading.
    constexpr unsigned long bucket =
        (Config::PressureChart::historyHours * 3600UL) / Config::PressureChart::barCount;

    constexpr int capacity = Config::PressureChart::barCount;

    // A timestamp sitting exactly on a bucket boundary.
    constexpr unsigned long base = 1736951400UL / bucket * bucket;
} // namespace

TEST_CASE("PressureHistory: startsEmpty")
{
    PressureHistory h;
    CHECK(h.count == 0);
}

TEST_CASE("PressureHistory: storesFirstReading")
{
    PressureHistory h;
    h.addReading(base, 1013.5f);

    CHECK(h.count == 1);
    CHECK(h.values[0] == doctest::Approx(1013.5f));
    CHECK(h.timestamps[0] == base);
}

TEST_CASE("PressureHistory: replacesWithinSameBucket")
{
    PressureHistory h;
    h.addReading(base, 1000.0f);
    h.addReading(base + bucket / 2, 1002.0f);

    CHECK(h.count == 1);
    CHECK(h.values[0] == doctest::Approx(1002.0f));
    CHECK(h.timestamps[0] == base + bucket / 2);
}

TEST_CASE("PressureHistory: appendsOnNextBucket")
{
    PressureHistory h;
    h.addReading(base, 1000.0f);
    h.addReading(base + bucket, 1001.0f);

    CHECK(h.count == 2);
    CHECK(h.values[0] == doctest::Approx(1000.0f));
    CHECK(h.values[1] == doctest::Approx(1001.0f));
}

TEST_CASE("PressureHistory: bucketsAreEpochAlignedNotRelative")
{
    // One second apart, but straddling a boundary -> two separate bars.
    PressureHistory h;
    h.addReading(base + bucket - 1, 1000.0f);
    h.addReading(base + bucket, 1001.0f);

    CHECK(h.count == 2);
}

TEST_CASE("PressureHistory: fillsToCapacity")
{
    PressureHistory h;
    for (int i = 0; i < capacity; i++)
    {
        h.addReading(base + (unsigned long)i * bucket, 1000.0f + i);
    }

    CHECK(h.count == capacity);
    CHECK(h.values[0] == doctest::Approx(1000.0f));
    CHECK(h.values[capacity - 1] == doctest::Approx(1000.0f + (capacity - 1)));
}

TEST_CASE("PressureHistory: dropsOldestWhenFull")
{
    PressureHistory h;
    for (int i = 0; i <= capacity; i++) // one past capacity
    {
        h.addReading(base + (unsigned long)i * bucket, 1000.0f + i);
    }

    CHECK(h.count == capacity);

    // Check every slot, not just the ends: a shift loop that mishandles the
    // middle leaves the endpoints correct and corrupts everything between.
    for (int i = 0; i < capacity; i++)
    {
        CHECK(h.values[i] == doctest::Approx(1000.0f + (i + 1)));
        CHECK(h.timestamps[i] == base + (unsigned long)(i + 1) * bucket);
    }
}

TEST_CASE("PressureHistory: reportsGapWhenEmpty")
{
    PressureHistory h;
    CHECK(h.hasGap(base, 3600));
}

TEST_CASE("PressureHistory: noGapWithinWindow")
{
    PressureHistory h;
    h.addReading(base, 1000.0f);

    CHECK(!h.hasGap(base + 3600, 3600));      // exactly at the limit is not a gap
    CHECK(h.hasGap(base + 3601, 3600));       // one second past is
}

TEST_CASE("PressureHistory: clearResetsCount")
{
    PressureHistory h;
    h.addReading(base, 1000.0f);
    h.addReading(base + bucket, 1001.0f);
    h.clear();

    CHECK(h.count == 0);
    CHECK(h.hasGap(base, 3600));
}
