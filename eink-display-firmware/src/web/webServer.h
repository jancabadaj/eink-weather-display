#pragma once

#include <WiFi.h>
#include "../provider/auth.h"
#include "../platform/arduino/displayManager.h"
#include "../weatherCore.h"
#include "../settings/configOverrides.h"

class WebServer
{
public:
    WebServer(std::shared_ptr<WeatherCore> weatherCore,
              std::shared_ptr<UpdateScheduler> scheduler,
              std::shared_ptr<DisplayManager> displayManager,
              std::shared_ptr<Auth> auth,
              std::shared_ptr<ConfigOverrides> configOverrides)
        : _weatherCore(weatherCore),
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

    static String formatDuration(unsigned long ms);
    static String parseQueryParam(const String &header, const String &key);
    static bool isValidInt(const String &s);

    std::shared_ptr<WeatherCore> _weatherCore;
    std::shared_ptr<UpdateScheduler> _scheduler;
    std::shared_ptr<DisplayManager> _displayManager;
    std::shared_ptr<Auth> _auth;
    std::shared_ptr<ConfigOverrides> _configOverrides;
};
