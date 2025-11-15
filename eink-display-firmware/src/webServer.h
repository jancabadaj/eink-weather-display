#pragma once
#include <WiFi.h>
#include "auth.h"
#include "weatherCore.h"

class WebServer
{
public:
    WebServer(std::shared_ptr<WeatherCore> weatherCore,
              std::shared_ptr<Auth> auth)
        : _auth(auth), _weatherCore(weatherCore) {}

    void init();
    void loop();

private:
    void handleRequest(WiFiClient &client);

    std::shared_ptr<WeatherCore> _weatherCore;
    std::shared_ptr<Auth> _auth;
};