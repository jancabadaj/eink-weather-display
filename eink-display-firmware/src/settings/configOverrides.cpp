#include <Arduino.h>
#include "configOverrides.h"
#include "logger.h"

static const char *NVS_NS = "cfg";
static const char *KEY_VERSION = "version";
static const char *KEY_NIGHT_START = "night_start";
static const char *KEY_NIGHT_END = "night_end";

void ConfigOverrides::init()
{
    Preferences prefs;
    prefs.begin(NVS_NS, false);

    String storedVersion = prefs.getString(KEY_VERSION, "");
    if (storedVersion != Config::version)
    {
        logger.warning("[ConfigOverrides] Version mismatch (stored: '%s', current: '%s') — clearing overrides",
                       storedVersion.c_str(), Config::version);
        prefs.clear();
        prefs.putString(KEY_VERSION, Config::version);
    }
    else
    {
        _nightStart = prefs.getInt(KEY_NIGHT_START, -1);
        _nightEnd = prefs.getInt(KEY_NIGHT_END, -1);
    }

    prefs.end();

    logger.info("[ConfigOverrides] Loaded — night: %s",
                (_nightStart != -1 || _nightEnd != -1)
                    ? (String(_nightStart) + "-" + String(_nightEnd)).c_str()
                    : "default");
}

void ConfigOverrides::resetAll()
{
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.clear();
    prefs.putString(KEY_VERSION, Config::version);
    prefs.end();

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
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putInt(KEY_NIGHT_START, hour);
    prefs.end();
    logger.info("[ConfigOverrides] Night start set to %d UTC", hour);
}

void ConfigOverrides::setNightEndHour(int hour)
{
    _nightEnd = hour;
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putInt(KEY_NIGHT_END, hour);
    prefs.end();
    logger.info("[ConfigOverrides] Night end set to %d UTC", hour);
}
