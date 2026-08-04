#pragma once

#include "render/screens.h"
#include "domain/weatherData.h"
#include "domain/pressureHistory.h"
#include "platform/arduino/displayManager.h"
#include "provider/auth.h"
#include "schedule/updateScheduler.h"
#include "schedule/serverClock.h"
#include <string>
#include <chrono>

class WeatherCore
{
public:
    WeatherCore(Auth &auth,
                Screens &renderer,
                DisplayManager &displayManager,
                UpdateScheduler &scheduler,
                ServerClock &serverClock)
        : _auth(auth), _renderer(renderer), _displayManager(displayManager), _serverClock(serverClock), _scheduler(scheduler) {}

    void loop();
    void reloadData();

    void restartUpdateLoop();
    bool isUpdateLoopStopped() const { return _updateLoopStopped; }

private:
    Auth &_auth;
    Screens &_renderer;
    DisplayManager &_displayManager;
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