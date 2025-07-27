#pragma once
#include <WiFi.h>
#include "weatherCore.h"

class WebServer
{
public:
    WebServer(std::shared_ptr<WeatherCore> weatherCore) : _weatherCore(weatherCore) {}

    void init();
    void loop();

private:
    void handleRequest(WiFiClient &client);

    std::shared_ptr<WeatherCore> _weatherCore;
};