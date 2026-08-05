#pragma once

#include <cstdint>

#include "../domain/pressureHistory.h"
#include "../domain/weatherData.h"

class WeatherProvider
{
public:
    virtual ~WeatherProvider() = default;

    virtual bool isAvailable() const = 0;

    // Renews credentials that are close to expiring
    virtual bool refreshCredentials() = 0;

    virtual bool fetchCurrent(WeatherData &out) = 0;

    // Pressure readings for the window ending at nowSec
    virtual bool fetchHistory(uint64_t nowSec, PressureHistory &out) = 0;
};
