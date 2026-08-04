#pragma once

#include <WiFi.h>

#include <string>
#include "../provider/auth.h"
#include "../platform/arduino/displayManager.h"
#include "../weatherCore.h"
#include "../settings/configOverrides.h"

class WebServer
{
public:
    WebServer(Clock &clock,
              WeatherCore &weatherCore,
              UpdateScheduler &scheduler,
              DisplayManager &displayManager,
              Auth &auth,
              ConfigOverrides &configOverrides)
        : _clock(clock), _weatherCore(weatherCore),
          _scheduler(scheduler),
          _displayManager(displayManager),
          _auth(auth),
          _configOverrides(configOverrides)
    {
    }

    void init();
    void loop();

private:
    // true => action endpoints, response should be a redirect
    // false => home page should be rendered
    bool handleRequest(WiFiClient &client);

    void sendHomePage(WiFiClient &client);

    static std::string formatDuration(uint64_t ms);
    static std::string parseQueryParam(const std::string &header, const std::string &key);
    static bool isValidInt(const std::string &s);

    Clock &_clock;
    WeatherCore &_weatherCore;
    UpdateScheduler &_scheduler;
    DisplayManager &_displayManager;
    Auth &_auth;
    ConfigOverrides &_configOverrides;
};
