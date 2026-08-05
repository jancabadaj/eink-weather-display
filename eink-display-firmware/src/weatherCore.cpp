
#include "weatherCore.h"
#include "provider/netatmoParse.h"
#include "config.h"
#include "logger.h"

void WeatherCore::loop()
{
    if (_updateLoopStopped || !_auth.isLoggedIn())
    {
        return;
    }

    // Fetch initial data on first run
    if (!_hasInitialData)
    {
        logger.info("[WeatherCore] Initial data fetch");
        reloadData();
        return;
    }

    // Check if it's time for scheduled refresh
    const uint64_t nextRefreshMillis = _scheduler.getNextScheduledRefreshMillis();
    if (_clock.uptimeMs() >= nextRefreshMillis)
    {
        logger.info("[WeatherCore] Scheduled refresh triggered");
        reloadData();
    }
}

void WeatherCore::restartUpdateLoop()
{
    if (_updateLoopStopped)
    {
        logger.info("[WeatherCore] Restarting update loop");
        _hasInitialData = false;
        _updateLoopStopped = false;
    }
}

void WeatherCore::reloadData()
{
    bool refresh = _auth.refreshTokenIfNeeded();
    if (!refresh)
    {
        logger.warning("[WeatherCore] Token refresh failed (consecutive failures: %d/%d)",
                       _consecutiveFailures, Config::Schedule::maxConsecutiveFailures);
        _consecutiveFailures++;
        updateDisplayAndSchedule();
        return;
    }

    const uint64_t requestStartMillis = _clock.uptimeMs();
    const HttpResponse response = _http.get(Config::Api::dataUrl, _auth.getAccessToken());

    if (response.ok())
    {
        logger.info("[WeatherCore] HTTP Response code: %d", response.status);

        parseAndUpdateWeatherData(response.body);

        _serverClock.syncTime(requestStartMillis, _weatherData.retrieval_timestamp);

        updatePressureHistory();

        _consecutiveFailures = 0;
        _hasInitialData = true;
    }
    else
    {
        logger.error("[WeatherCore] HTTP error (consecutive failures: %d/%d) %d: %s",
                     _consecutiveFailures, Config::Schedule::maxConsecutiveFailures, response.status, response.body.c_str());
        _consecutiveFailures++;
    }

    updateDisplayAndSchedule();
}

void WeatherCore::parseAndUpdateWeatherData(const std::string &payload)
{
    NetatmoParse::StationData parsed;
    if (!NetatmoParse::parseStationData(payload, parsed))
    {
        return;
    }

    _weatherData = parsed.weather;
    if (!parsed.deviceId.empty())
    {
        _deviceId = parsed.deviceId;
    }
}

void WeatherCore::updateDisplayAndSchedule()
{
    if (_consecutiveFailures >= Config::Schedule::maxConsecutiveFailures)
    {
        logger.error("[WeatherCore] Max consecutive failures reached! Clearing display and stopping updates.");
        _renderer.renderNetworkError();
        _display.present(_renderer.pixels());
        _updateLoopStopped = true;
        return;
    }

    if (_consecutiveFailures > 0)
    {
        logger.info("[WeatherCore] Failure detected, retry after delay");
        _scheduler.scheduleRetry();
        return;
    }

    // Schedule next refresh based on data timestamp
    bool scheduled = _scheduler.scheduleRefresh(_weatherData.data_timestamp.count());
    bool dataChanged = (_weatherData.data_timestamp != _previousDataTimestamp);
    _previousDataTimestamp = _weatherData.data_timestamp;

    if (!scheduled)
    {
        // Night mode - always update display to show night mode indicator
        logger.info("[WeatherCore] Next refresh scheduled during night time, updates paused");
        _renderer.renderNightModeIndicator();
        _display.present(_renderer.pixels());
        logger.info("[WeatherCore] Display set to night mode");
    }
    else
    {
        // Skip display update if data unchanged
        if (!dataChanged)
        {
            logger.info("[WeatherCore] Data unchanged (timestamp: %lld), skipping display refresh",
                        _weatherData.data_timestamp.count());
            return;
        }

        // Update display with new data
        logger.info("[WeatherCore] Updating display. [Data timestamp: %lld, Server time: %lld]",
                    _weatherData.data_timestamp.count(),
                    _weatherData.retrieval_timestamp.count());
        _renderer.renderWeather(_weatherData, _pressureHistory);
        _display.present(_renderer.pixels());
        logger.info("[WeatherCore] Display updated");
    }
}

void WeatherCore::updatePressureHistory()
{
    unsigned long nowSec = _weatherData.retrieval_timestamp.count() / 1000;
    unsigned long dataTimestampSec = _weatherData.data_timestamp.count() / 1000;
    if (nowSec == 0 || dataTimestampSec == 0 || _deviceId.length() == 0)
        return;

    unsigned long maxGap = (Config::PressureChart::historyHours * 3600UL) / Config::PressureChart::barCount;
    if (_pressureHistory.hasGap(nowSec, maxGap))
    {
        fetchPressureHistory(nowSec);
    }
    _pressureHistory.addReading(dataTimestampSec, _weatherData.internal.pressure);
}

void WeatherCore::fetchPressureHistory(unsigned long nowSec)
{
    unsigned long dateBegin = nowSec - (Config::PressureChart::historyHours * 3600UL);

    std::string url = Config::Api::historyUrl;
    url += "?device_id=" + _deviceId;
    url += "&date_begin=" + std::to_string(dateBegin);
    url += "&scale=1hour";
    url += "&type=pressure";
    url += "&optimize=false";
    url += "&real_time=false";

    const HttpResponse response = _http.get(url, _auth.getAccessToken());
    if (!response.ok())
    {
        logger.error("[WeatherCore] Pressure history fetch failed: %d", response.status);
        return;
    }

    if (!NetatmoParse::parseMeasure(response.body, _pressureHistory))
    {
        return;
    }

    logger.info("[WeatherCore] Pressure history loaded: %d entries", _pressureHistory.count);
}
