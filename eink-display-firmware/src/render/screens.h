#pragma once

#include <memory>

#include "../domain/weatherData.h"
#include "../domain/pressureHistory.h"

#include "color.h"

class WeatherRenderer
{
public:
    WeatherRenderer(uint8_t *imageData) : _imageData(imageData) {}

    void renderWeather(const WeatherData &data, const PressureHistory &pressureHistory);
    void renderNightModeIndicator();
    void renderNetworkError();

private:
    uint8_t *_imageData;
};