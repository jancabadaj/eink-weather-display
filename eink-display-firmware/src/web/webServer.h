#pragma once

#include <WiFi.h>

#include <string>
#include "../provider/auth.h"
#include "../platform/displayPanel.h"
#include "../platform/network.h"
#include "../weatherCore.h"
#include "../settings/configOverrides.h"

class WebServer
{
public:
    WebServer(Clock &clock,
              Network &network,
              WeatherCore &weatherCore,
              UpdateScheduler &scheduler,
              DisplayPanel &display,
              Auth &auth,
              ConfigOverrides &configOverrides)
        : _clock(clock), _network(network), _weatherCore(weatherCore),
          _scheduler(scheduler),
          _display(display),
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
    Network &_network;
    WeatherCore &_weatherCore;
    UpdateScheduler &_scheduler;
    DisplayPanel &_display;
    Auth &_auth;
    ConfigOverrides &_configOverrides;
};
