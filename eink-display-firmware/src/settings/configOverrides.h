#pragma once

#include <Preferences.h>

class ConfigOverrides
{
public:
    void init();

    void resetAll();

    int getNightStartHour() const;
    int getNightEndHour() const;

    bool hasNightOverride() const { return _nightStart != -1 || _nightEnd != -1; }

    void setNightStartHour(int hour);
    void setNightEndHour(int hour);

private:
    int _nightStart = -1;
    int _nightEnd = -1;
};
