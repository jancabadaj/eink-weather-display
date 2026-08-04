#pragma once

#include "render/screens.h"
#include "domain/weatherData.h"
#include "domain/pressureHistory.h"
#include "platform/displayPanel.h"
#include "render/frameBuffer.h"
#include "platform/clock.h"
#include "platform/httpClient.h"
#include "provider/auth.h"
#include "schedule/updateScheduler.h"
#include "schedule/serverClock.h"
#include <string>
#include <chrono>

class WeatherCore
{
public:
    WeatherCore(Clock &clock,
                HttpClient &http,
                Auth &auth,
                Screens &renderer,
                DisplayPanel &display,
                UpdateScheduler &scheduler,
                ServerClock &serverClock)
        : _clock(clock), _http(http), _auth(auth), _renderer(renderer), _display(display), _serverClock(serverClock), _scheduler(scheduler) {}

    void loop();
    void reloadData();

    void restartUpdateLoop();
    bool isUpdateLoopStopped() const { return _updateLoopStopped; }

private:
    Clock &_clock;
    HttpClient &_http;
    Auth &_auth;
    Screens &_renderer;
    DisplayPanel &_display;
    ServerClock &_serverClock;
    UpdateScheduler &_scheduler;

    void parseAndUpdateWeatherData(const std::string &payload);
    void updateDisplayAndSchedule();
    void updatePressureHistory();
    void fetchPressureHistory(unsigned long nowSec);

    WeatherData _weatherData{};
    PressureHistory _pressureHistory;
    std::string _deviceId;
    std::chrono::milliseconds _previousDataTimestamp{0};
    bool _hasInitialData = false;
    bool _updateLoopStopped = false;
    int _consecutiveFailures = 0;
};