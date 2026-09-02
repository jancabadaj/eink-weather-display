#pragma once

#include <chrono>

#include "domain/pressureHistory.h"
#include "domain/weatherData.h"
#include "platform/clock.h"
#include "platform/displayPanel.h"
#include "provider/weatherProvider.h"
#include "render/screens.h"
#include "schedule/serverClock.h"
#include "schedule/updateScheduler.h"

// The core application logic
class App
{
public:
    App(Clock &clock,
        WeatherProvider &provider,
        Screens &renderer,
        DisplayPanel &display,
        UpdateScheduler &scheduler,
        ServerClock &serverClock)
        : _clock(clock), _provider(provider), _renderer(renderer), _display(display),
          _scheduler(scheduler), _serverClock(serverClock) {}

    // Called from the main loop; returns immediately when nothing is due
    void tick();

    // Fetch new data immediatelly, regardless of the schedule
    void reloadData();

    void restartUpdateLoop();
    bool isUpdateLoopStopped() const { return _updatesStopped; }

private:
    void updateDisplayAndSchedule();
    void updatePressureHistory();

    Clock &_clock;
    WeatherProvider &_provider;
    Screens &_renderer;
    DisplayPanel &_display;
    UpdateScheduler &_scheduler;
    ServerClock &_serverClock;

    WeatherData _weatherData{};
    PressureHistory _pressureHistory;
    std::chrono::milliseconds _previousDataTimestamp{0};
    bool _hasInitialData = false;
    bool _updatesStopped = false;
    int _consecutiveFailures = 0;
};
