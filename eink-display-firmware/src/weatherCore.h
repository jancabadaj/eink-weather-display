#pragma once

#include "render/screens.h"
#include "domain/weatherData.h"
#include "domain/pressureHistory.h"
#include "platform/arduino/displayManager.h"
#include "provider/auth.h"
#include "schedule/updateScheduler.h"
#include "schedule/serverClock.h"
#include <memory>
#include <chrono>

class WeatherCore
{
public:
    WeatherCore(std::shared_ptr<Auth> auth,
                std::shared_ptr<Screens> renderer,
                std::shared_ptr<DisplayManager> displayManager,
                std::shared_ptr<UpdateScheduler> scheduler,
                std::shared_ptr<ServerClock> serverClock)
        : _auth(auth), _renderer(renderer), _displayManager(displayManager), _serverClock(serverClock), _scheduler(scheduler) {}

    void loop();
    void reloadData();

    void restartUpdateLoop();
    bool isUpdateLoopStopped() const { return _updateLoopStopped; }

private:
    std::shared_ptr<Auth> _auth;
    std::shared_ptr<Screens> _renderer;
    std::shared_ptr<DisplayManager> _displayManager;
    std::shared_ptr<ServerClock> _serverClock;
    std::shared_ptr<UpdateScheduler> _scheduler;

    void parseAndUpdateWeatherData(const String &payload);
    void updateDisplayAndSchedule();
    void updatePressureHistory();
    void fetchPressureHistory(unsigned long nowSec);

    WeatherData _weatherData{};
    PressureHistory _pressureHistory;
    String _deviceId;
    std::chrono::milliseconds _previousDataTimestamp{0};
    bool _hasInitialData = false;
    bool _updateLoopStopped = false;
    int _consecutiveFailures = 0;
};