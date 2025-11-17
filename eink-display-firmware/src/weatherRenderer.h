#pragma once

#include <memory>

#include "weatherData.h"

#include "draw/color.h"

class WeatherRenderer
{
public:
    WeatherRenderer(uint8_t *imageData) : _imageData(imageData) {}

    void renderWeather(const WeatherData &data);
    void renderNightModeIndicator();
    void renderNetworkError();

private:
    uint8_t *_imageData;
};