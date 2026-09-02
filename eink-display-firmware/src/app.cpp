#include "app.h"

#include "config.h"
#include "logger.h"

void App::tick()
{
    if (_updatesStopped || !_provider.isAvailable())
    {
        return;
    }

    // Fetch initial data on first run
    if (!_hasInitialData)
    {
        logger.info("[App] Initial data fetch");
        reloadData();
        return;
    }

    // Check if it's time for scheduled refresh
    if (_clock.uptimeMs() >= _scheduler.getNextScheduledRefreshMillis())
    {
        logger.info("[App] Scheduled refresh triggered");
        reloadData();
    }
}

void App::restartUpdateLoop()
{
    if (_updatesStopped)
    {
        logger.info("[App] Restarting update loop");
        _hasInitialData = false;
        _updatesStopped = false;
    }
}

void App::reloadData()
{
    if (!_provider.refreshCredentials())
    {
        logger.warning("[App] Credential refresh failed (consecutive failures: %d/%d)",
                       _consecutiveFailures, Config::Schedule::maxConsecutiveFailures);
        _consecutiveFailures++;
        updateDisplayAndSchedule();
        return;
    }

    const uint64_t requestStartMillis = _clock.uptimeMs();

    WeatherData fetched{};
    if (_provider.fetchCurrent(fetched))
    {
        _weatherData = fetched;
        _serverClock.syncTime(requestStartMillis, _weatherData.retrieval_timestamp);
        updatePressureHistory();

        _consecutiveFailures = 0;
        _hasInitialData = true;
    }
    else
    {
        logger.error("[App] Fetch failed (consecutive failures: %d/%d)", _consecutiveFailures,
                     Config::Schedule::maxConsecutiveFailures);
        _consecutiveFailures++;
    }

    updateDisplayAndSchedule();
}

void App::updateDisplayAndSchedule()
{
    if (_consecutiveFailures >= Config::Schedule::maxConsecutiveFailures)
    {
        logger.error("[App] Max consecutive failures reached! Clearing display and stopping updates.");
        _renderer.renderNetworkError();
        _display.present(_renderer.pixels());
        _updatesStopped = true;
        return;
    }

    if (_consecutiveFailures > 0)
    {
        logger.info("[App] Failure detected, retry after delay");
        _scheduler.scheduleRetry();
        return;
    }

    // Schedule next refresh based on data timestamp
    const Planner::Mode mode = _scheduler.scheduleRefresh(_weatherData.data_timestamp.count());
    const bool dataChanged = (_weatherData.data_timestamp != _previousDataTimestamp);
    _previousDataTimestamp = _weatherData.data_timestamp;

    if (mode == Planner::Mode::Night)
    {
        logger.info("[App] Next refresh scheduled during night time, updates paused");
        _renderer.renderNightModeIndicator();
        _display.present(_renderer.pixels());
        return;
    }

    if (!dataChanged)
    {
        logger.info("[App] Data unchanged (timestamp: %lld), skipping display refresh",
                    _weatherData.data_timestamp.count());
        return;
    }

    logger.info("[App] Updating display. [Data timestamp: %lld, Server time: %lld]",
                _weatherData.data_timestamp.count(), _weatherData.retrieval_timestamp.count());
    _renderer.renderWeather(_weatherData, _pressureHistory);
    _display.present(_renderer.pixels());
}

void App::updatePressureHistory()
{
    const uint64_t nowSec = _weatherData.retrieval_timestamp.count() / 1000;
    const uint64_t dataTimestampSec = _weatherData.data_timestamp.count() / 1000;
    if (nowSec == 0 || dataTimestampSec == 0)
    {
        return;
    }

    constexpr unsigned long maxGap =
        (Config::PressureChart::historyHours * 3600UL) / Config::PressureChart::barCount;

    if (_pressureHistory.hasGap(nowSec, maxGap))
    {
        _provider.fetchHistory(nowSec, _pressureHistory);
    }
    _pressureHistory.addReading(dataTimestampSec, _weatherData.internal.pressure);
}
