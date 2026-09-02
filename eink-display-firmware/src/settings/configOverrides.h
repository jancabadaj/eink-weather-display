#pragma once

#include "../platform/storage.h"

// Runtime overrides of the compile-time defaults, persisted across reboots
// Stored together with a version string to detect incompatible changes in config structure, which clears the overrides
class ConfigOverrides
{
public:
    explicit ConfigOverrides(Storage &storage) : _storage(storage) {}

    void init();

    void resetAll();

    int getNightStartHour() const;
    int getNightEndHour() const;

    bool hasNightOverride() const { return _nightStart != -1 || _nightEnd != -1; }

    void setNightStartHour(int hour);
    void setNightEndHour(int hour);

private:
    Storage &_storage;
    int _nightStart = -1;
    int _nightEnd = -1;
};
