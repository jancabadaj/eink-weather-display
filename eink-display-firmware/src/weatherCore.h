#pragma once

#include "weatherRenderer.h"
#include "weatherData.h"
#include "hw/displayManager.h"
#include <memory>

struct AuthData
{
    String accessToken;
    String refreshToken;
    unsigned long tokenExpirationTime;
};

class WeatherCore
{
public:
    WeatherCore(std::shared_ptr<WeatherRenderer> renderer,
                std::shared_ptr<DisplayManager> displayManager)
        : _renderer(renderer), _displayManager(displayManager) {}

    void loop();

    void authenticate(const String &code);
    AuthData const &getAuthData() const;

    void reloadData();

    void clearDisplay();    // TODO: Temporary debug, delete it
    void drawWeatherData(); // TODO: Temporary debug, delete it

private:
    std::shared_ptr<WeatherRenderer> _renderer;
    std::shared_ptr<DisplayManager> _displayManager;

    void parseWeatherData(const String &payload);

    AuthData authData;
    WeatherData weatherData;
};