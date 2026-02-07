#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "weatherCore.h"
#include "config.h"
#include "logger.h"

void WeatherCore::loop()
{
    if (_updateLoopStopped || !_auth->isLoggedIn())
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
    unsigned long nextRefreshMillis = _scheduler->getNextScheduledRefreshMillis();
    if (millis() >= nextRefreshMillis)
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
    bool refresh = _auth->refreshTokenIfNeeded();
    if (!refresh)
    {
        logger.warning("[WeatherCore] Token refresh failed, cannot reload data");
        _renderer->renderNetworkError();
        _displayManager->refreshDisplay();
        logger.info("[WeatherCore] Display set to network error");
        return;
    }

    HTTPClient http;
    http.begin(Config::Api::dataUrl);
    http.addHeader("Authorization", "Bearer " + _auth->getAccessToken());

    // Send HTTP GET request
    unsigned long requestStartMillis = millis();
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200)
    {
        logger.info("[WeatherCore] HTTP Response code: %d", httpResponseCode);
        String payload = http.getString();

        parseAndUpdateWeatherData(payload);

        _serverClock->syncTime(requestStartMillis, _weatherData.retrieval_timestamp);

        _consecutiveFailures = 0;
        _hasInitialData = true;
    }
    else
    {
        logger.error("[WeatherCore] HTTP error (consecutive failures: %d/%d) %d: %s",
                     _consecutiveFailures, Config::Schedule::maxConsecutiveFailures, httpResponseCode, http.getString().c_str());
        _consecutiveFailures++;
    }

    updateDisplayAndSchedule();

    // Free resources
    http.end();
}

void WeatherCore::parseAndUpdateWeatherData(const String &payload)
{
    StaticJsonDocument<4096> doc; // Max size of expected payload
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        logger.error("[WeatherCore] deserializeJson() failed: %s", error.f_str());
        return;
    }

    // Navigate to the first device (main station)
    JsonObject device = doc["body"]["devices"][0];

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
        _displayManager->clearDisplay();
        _updateLoopStopped = true;
        return;
    }

    // Schedule next refresh
    bool scheduled = _scheduler->scheduleRefresh(_weatherData.data_timestamp.count());
    bool dataChanged = (_weatherData.data_timestamp != _previousDataTimestamp);
    _previousDataTimestamp = _weatherData.data_timestamp;

    if (!scheduled)
    {
        // Night mode - always update display to show night mode indicator
        logger.info("[WeatherCore] Next refresh scheduled during night time, updates paused");
        _renderer->renderNightModeIndicator();
        _displayManager->refreshDisplay();
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
        _renderer->renderWeather(_weatherData);
        _displayManager->refreshDisplay();
        logger.info("[WeatherCore] Display updated");
    }
}
