#include <ArduinoJson.h>

#include "weatherCore.h"
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
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        logger.error("[WeatherCore] deserializeJson() failed: %s", error.c_str());
        return;
    }

    // Navigate to the first device (main station)
    JsonObject device = doc["body"]["devices"][0];

    // Extract device ID for history API
    const char *deviceId = device["_id"] | "";
    if (strlen(deviceId) > 0)
    {
        _deviceId = deviceId;
    }

    // Internal data (main station)
    JsonObject internalDash = device["dashboard_data"];
    WeatherDataInternal internal;
    internal.temperature = internalDash["Temperature"] | 0.0f;
    internal.humidity = internalDash["Humidity"] | 0;
    internal.pressure = internalDash["Pressure"] | 0.0f;
    internal.noise = internalDash["Noise"] | 0;
    internal.co2 = internalDash["CO2"] | 0;

    // External data (first outdoor module)
    JsonObject module = device["modules"][0];
    JsonObject externalDash = module["dashboard_data"];
    WeatherDataExternal external;
    external.temperature = externalDash["Temperature"] | 0.0f;
    external.humidity = externalDash["Humidity"] | 0;

    // Timestamp (use internal time_utc)
    unsigned long long data_utc = internalDash["time_utc"] | 0;
    std::chrono::system_clock::time_point data_timestamp = std::chrono::system_clock::from_time_t(data_utc);

    unsigned long long retrieval_utc = doc["time_server"] | 0;
    std::chrono::system_clock::time_point retrieval_timestamp = std::chrono::system_clock::from_time_t(retrieval_utc);

    _weatherData = {
        .internal = internal,
        .external = external,
        .data_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(data_timestamp.time_since_epoch()),
        .retrieval_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(retrieval_timestamp.time_since_epoch())};
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

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response.body);
    if (error)
    {
        logger.error("[WeatherCore] Pressure history JSON parse failed: %s", error.c_str());
        return;
    }

    _pressureHistory.clear();
    for (JsonPair kv : doc["body"].as<JsonObject>())
    {
        unsigned long timestamp = strtoul(kv.key().c_str(), nullptr, 10);
        JsonArray vals = kv.value().as<JsonArray>();
        if (vals.size() == 0)
            continue;
        _pressureHistory.addReading(timestamp, vals[0] | 0.0f);
    }

    logger.info("[WeatherCore] Pressure history loaded: %d entries", _pressureHistory.count);
}
