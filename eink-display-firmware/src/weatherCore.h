#pragma once

#include "weatherRenderer.h"
#include "weatherData.h"
#include "hw/displayManager.h"
#include "auth.h"
#include <memory>

class WeatherCore
{
public:
    WeatherCore(std::shared_ptr<Auth> auth,
                std::shared_ptr<WeatherRenderer> renderer,
                std::shared_ptr<DisplayManager> displayManager)
        : _auth(auth), _renderer(renderer), _displayManager(displayManager) {}

    void loop();

    void reloadData();

    void clearDisplay();    // TODO: Temporary debug, delete it
    void drawWeatherData(); // TODO: Temporary debug, delete it

private:
    std::shared_ptr<Auth> _auth;
    std::shared_ptr<WeatherRenderer> _renderer;
    std::shared_ptr<DisplayManager> _displayManager;

    void parseWeatherData(const String &payload);

    WeatherData weatherData;
};