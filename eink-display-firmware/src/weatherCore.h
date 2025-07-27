#pragma once

#include "displayRenderer.h"
#include <memory>

class WeatherCore
{
public:
    WeatherCore(std::shared_ptr<DisplayRenderer> renderer) : _renderer(renderer) {}

    void loop();

    void clearDisplay();
    void drawWeatherData();

private:
    std::shared_ptr<DisplayRenderer> _renderer;
};