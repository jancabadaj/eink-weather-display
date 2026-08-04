#include <string>

#include "../config.h"
#include "../logger.h"
#include "configOverrides.h"

static const char *KEY_VERSION = "version";
static const char *KEY_NIGHT_START = "night_start";
static const char *KEY_NIGHT_END = "night_end";

void ConfigOverrides::init()
{
    const std::string storedVersion = _storage.getString(KEY_VERSION, "");
    if (storedVersion != Config::version)
    {
        logger.warning("[ConfigOverrides] Version mismatch (stored: '%s', current: '%s') — clearing overrides",
                       storedVersion.c_str(), Config::version);
        _storage.clear();
        _storage.putString(KEY_VERSION, Config::version);
    }
    else
    {
        _nightStart = _storage.getInt(KEY_NIGHT_START, -1);
        _nightEnd = _storage.getInt(KEY_NIGHT_END, -1);
    }

    logger.info("[ConfigOverrides] Loaded — night: %s",
                (_nightStart != -1 || _nightEnd != -1)
                    ? (std::to_string(_nightStart) + "-" + std::to_string(_nightEnd)).c_str()
                    : "default");
}

void ConfigOverrides::resetAll()
{
    _storage.clear();
    _storage.putString(KEY_VERSION, Config::version);

    _nightStart = -1;
    _nightEnd = -1;
    logger.info("[ConfigOverrides] All overrides reset to firmware defaults");
}

int ConfigOverrides::getNightStartHour() const
{
    return _nightStart != -1 ? _nightStart : Config::Schedule::nightStartHourUtc;
}

int ConfigOverrides::getNightEndHour() const
{
    return _nightEnd != -1 ? _nightEnd : Config::Schedule::nightEndHourUtc;
}

void ConfigOverrides::setNightStartHour(int hour)
{
    _nightStart = hour;
    _storage.putInt(KEY_NIGHT_START, hour);
    logger.info("[ConfigOverrides] Night start set to %d UTC", hour);
}

void ConfigOverrides::setNightEndHour(int hour)
{
    _nightEnd = hour;
    _storage.putInt(KEY_NIGHT_END, hour);
    logger.info("[ConfigOverrides] Night end set to %d UTC", hour);
}
