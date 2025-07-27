#include "weatherCore.h"

#include <Arduino.h>
#include <ArduinoJson.h>

void WeatherCore::loop()
{
    // Here handle the logic of fetching weather data, processing it, and rendering it
}

// TODO: Temporary debug, delete it
void WeatherCore::clearDisplay()
{
    _displayManager->clearDisplay();
}

// TODO: Temporary debug, delete it
void WeatherCore::drawWeatherData()
{
    // This function can be used to draw weather data on the display
    WeatherData data = {
        .internal = {25.0f, 60, 1013.3f, 30, 400},
        .external = {22.0f, 55},
        .timestamp = std::chrono::system_clock::now()};

    _renderer->renderWeather(data);
    _displayManager->refreshDisplay();
}