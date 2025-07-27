#pragma once

#include "weatherRenderer.h"
#include "hw/displayManager.h"
#include <memory>

class WeatherCore
{
public:
    WeatherCore(std::shared_ptr<WeatherRenderer> renderer,
                std::shared_ptr<DisplayManager> displayManager)
        : _renderer(renderer), _displayManager(displayManager) {}

    void loop();

    void clearDisplay();    // TODO: Temporary debug, delete it
    void drawWeatherData(); // TODO: Temporary debug, delete it

private:
    std::shared_ptr<WeatherRenderer> _renderer;
    std::shared_ptr<DisplayManager> _displayManager;
};