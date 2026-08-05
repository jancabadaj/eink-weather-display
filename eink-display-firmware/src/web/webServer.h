#pragma once

#include "../platform/clock.h"
#include "../platform/displayPanel.h"
#include "../platform/network.h"
#include "../provider/auth.h"
#include "../schedule/updateScheduler.h"
#include "../settings/configOverrides.h"
#include "../weatherCore.h"
#include "requestHandler.h"
#include "statusSnapshot.h"

// The device's admin web UI
// Not responsible for the transport, just handling requests and producing responses
class WebServer : public RequestHandler
{
public:
    WebServer(Clock &clock,
              Network &network,
              WeatherCore &weatherCore,
              UpdateScheduler &scheduler,
              DisplayPanel &display,
              Auth &auth,
              ConfigOverrides &configOverrides)
        : _clock(clock), _network(network), _weatherCore(weatherCore), _scheduler(scheduler),
          _display(display), _auth(auth), _configOverrides(configOverrides) {}

    Web::Response handle(const Web::Request &request) override;

    StatusSnapshot captureStatus() const;

private:
    Web::Response applyConfig(const Web::Request &request);

    Clock &_clock;
    Network &_network;
    WeatherCore &_weatherCore;
    UpdateScheduler &_scheduler;
    DisplayPanel &_display;
    Auth &_auth;
    ConfigOverrides &_configOverrides;
};
