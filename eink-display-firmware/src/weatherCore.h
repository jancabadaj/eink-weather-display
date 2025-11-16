#pragma once

#include "weatherRenderer.h"
#include "weatherData.h"
#include "hw/displayManager.h"
#include "auth.h"
#include "updateSchedule.h"
#include <memory>
#include <chrono>

class WeatherCore
{
public:
    WeatherCore(std::shared_ptr<Auth> auth,
                std::shared_ptr<WeatherRenderer> renderer,
                std::shared_ptr<DisplayManager> displayManager)
        : _auth(auth), _renderer(renderer), _displayManager(displayManager), _scheduler(std::make_shared<UpdateScheduler>()) {}

    void loop();
    void reloadData();

    void restartUpdateLoop();
    bool isUpdateLoopStopped() const { return _updateLoopStopped; }

private:
    std::shared_ptr<Auth> _auth;
    std::shared_ptr<WeatherRenderer> _renderer;
    std::shared_ptr<DisplayManager> _displayManager;
    std::shared_ptr<UpdateScheduler> _scheduler;

    void parseWeatherData(const String &payload);
    unsigned long getCurrentUtcTimeMillis();
    void handleRefreshSuccess(unsigned long intervalMs);
    void handleRefreshFailure();

    WeatherData weatherData;
    unsigned long _lastRefreshAttemptMillis = 0;
    bool _hasInitialData = false;
    int _consecutiveFailures = 0;
    bool _updateLoopStopped = false;
};