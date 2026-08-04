#include <doctest.h>

#include "config.h"
#include "fakes/memStorage.h"
#include "settings/configOverrides.h"

TEST_CASE("ConfigOverrides: fallsBackToFirmwareDefaults")
{
    MemStorage storage;
    ConfigOverrides overrides(storage);
    overrides.init();

    CHECK(overrides.getNightStartHour() == Config::Schedule::nightStartHourUtc);
    CHECK(overrides.getNightEndHour() == Config::Schedule::nightEndHourUtc);
    CHECK(overrides.hasNightOverride() == false);
}

TEST_CASE("ConfigOverrides: storesAndReportsAnOverride")
{
    MemStorage storage;
    ConfigOverrides overrides(storage);
    overrides.init();

    overrides.setNightStartHour(22);

    CHECK(overrides.getNightStartHour() == 22);
    CHECK(overrides.hasNightOverride());
    CHECK(storage.hasInt("night_start"));
    // The other half is still the firmware default.
    CHECK(overrides.getNightEndHour() == Config::Schedule::nightEndHourUtc);
}

TEST_CASE("ConfigOverrides: reloadsPersistedValues")
{
    MemStorage storage;
    {
        ConfigOverrides first(storage);
        first.init();
        first.setNightStartHour(23);
        first.setNightEndHour(7);
    }

    ConfigOverrides second(storage);
    second.init();

    CHECK(second.getNightStartHour() == 23);
    CHECK(second.getNightEndHour() == 7);
}

TEST_CASE("ConfigOverrides: versionMismatchClearsStoredOverrides")
{
    MemStorage storage;
    storage.putString("version", "an-older-firmware");
    storage.putInt("night_start", 22);

    ConfigOverrides overrides(storage);
    overrides.init();

    CHECK(storage.clears == 1);
    CHECK(overrides.hasNightOverride() == false);
    CHECK(overrides.getNightStartHour() == Config::Schedule::nightStartHourUtc);
    // The current version is written back so the next boot keeps its overrides.
    CHECK(storage.getString("version", "") == Config::version);
}

TEST_CASE("ConfigOverrides: matchingVersionKeepsStoredOverrides")
{
    MemStorage storage;
    storage.putString("version", Config::version);
    storage.putInt("night_start", 22);
    storage.putInt("night_end", 6);

    ConfigOverrides overrides(storage);
    overrides.init();

    CHECK(storage.clears == 0);
    CHECK(overrides.getNightStartHour() == 22);
    CHECK(overrides.getNightEndHour() == 6);
}

TEST_CASE("ConfigOverrides: resetAllRestoresDefaults")
{
    MemStorage storage;
    ConfigOverrides overrides(storage);
    overrides.init();
    overrides.setNightStartHour(22);

    overrides.resetAll();

    CHECK(overrides.hasNightOverride() == false);
    CHECK(overrides.getNightStartHour() == Config::Schedule::nightStartHourUtc);
    CHECK(storage.getString("version", "") == Config::version);
}
