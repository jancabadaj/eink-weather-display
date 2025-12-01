#pragma once

#include "weatherRenderer.h"
#include "weatherData.h"
#include "hw/displayManager.h"
#include "auth.h"
#include "updateScheduler.h"
#include "serverClock.h"
#include <memory>
#include <chrono>

class WeatherCore
{
public:
    WeatherCore(std::shared_ptr<Auth> auth,
                std::shared_ptr<WeatherRenderer> renderer,
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
    std::shared_ptr<WeatherRenderer> _renderer;
    std::shared_ptr<DisplayManager> _displayManager;
    std::shared_ptr<ServerClock> _serverClock;
    std::shared_ptr<UpdateScheduler> _scheduler;

    void parseAndUpdateWeatherData(const String &payload);
    void handleRefreshSuccess();
    void handleRefreshFailure();

    WeatherData _weatherData;
    bool _hasInitialData = false;
    bool _updateLoopStopped = false;
};