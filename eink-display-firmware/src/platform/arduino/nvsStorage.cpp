#include "nvsStorage.h"

std::string NvsStorage::getString(const char *key, const char *fallback)
{
    Preferences prefs;
    prefs.begin(_ns, true);
    const std::string value = prefs.getString(key, fallback).c_str();
    prefs.end();
    return value;
}

void NvsStorage::putString(const char *key, const std::string &value)
{
    Preferences prefs;
    prefs.begin(_ns, false);
    prefs.putString(key, value.c_str());
    prefs.end();
}

int NvsStorage::getInt(const char *key, int fallback)
{
    Preferences prefs;
    prefs.begin(_ns, true);
    const int value = prefs.getInt(key, fallback);
    prefs.end();
    return value;
}

void NvsStorage::putInt(const char *key, int value)
{
    Preferences prefs;
    prefs.begin(_ns, false);
    prefs.putInt(key, value);
    prefs.end();
}

void NvsStorage::clear()
{
    Preferences prefs;
    prefs.begin(_ns, false);
    prefs.clear();
    prefs.end();
}
