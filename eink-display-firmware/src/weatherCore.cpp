#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "weatherCore.h"
#include "config.h"
#include "definitions.h"
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
        _consecutiveFailures = 0;
        _hasInitialData = false;
        _updateLoopStopped = false;
    }
}

void WeatherCore::reloadData()
{
    _auth->refreshTokenIfNeeded();
    AuthData authData = _auth->getAuthData();

    HTTPClient http;
    http.begin(NETATMO_SERVER_DATA);
    http.addHeader("Authorization", "Bearer " + authData.accessToken);

    // Send HTTP GET request
    unsigned long refreshAttemptMillis = millis();
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
        logger.info("[WeatherCore] HTTP Response code: %d", httpResponseCode);
        String payload = http.getString();

        // Store previous data timestamp for interval calculation
        auto previousDataTimestamp = weatherData.data_timestamp;

        parseWeatherData(payload);

        // Sync clock with server time
        _serverClock->syncTime(refreshAttemptMillis, weatherData.retrieval_timestamp);

        // Calculate data update interval if we have previous data
        if (_hasInitialData && previousDataTimestamp.time_since_epoch().count() > 0)
        {
            auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
                weatherData.data_timestamp - previousDataTimestamp);

            if (timeDiff.count() > 0)
            {
                unsigned long intervalMs = timeDiff.count();
                handleRefreshSuccess(intervalMs);
            }
        }
        else
        {
            // First data fetch
            handleRefreshSuccess(0);
        }

        _hasInitialData = true;

        // Update display with new data
        logger.info("[WeatherCore] Updating display. [Data timestamp: %lld, Server time: %lld]",
                    std::chrono::duration_cast<std::chrono::seconds>(
                        weatherData.data_timestamp.time_since_epoch())
                        .count(),
                    std::chrono::duration_cast<std::chrono::seconds>(
                        weatherData.retrieval_timestamp.time_since_epoch())
                        .count());
        _renderer->renderWeather(weatherData);
        _displayManager->refreshDisplay();
        logger.info("[WeatherCore] Display updated");
    }
    else
    {
        logger.error("[WeatherCore] HTTP error code: %d", httpResponseCode);
        handleRefreshFailure();
    }

    // Free resources
    http.end();
}

void WeatherCore::parseWeatherData(const String &payload)
{
    StaticJsonDocument<4096> doc; // Adjust size as needed
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
    long data_utc = internalDash["time_utc"] | 0;
    std::chrono::system_clock::time_point data_timestamp = std::chrono::system_clock::from_time_t(data_utc);

    long retrieval_utc = doc["time_server"] | 0;
    std::chrono::system_clock::time_point retrieval_timestamp = std::chrono::system_clock::from_time_t(retrieval_utc);

    weatherData = {
        .internal = internal,
        .external = external,
        .data_timestamp = data_timestamp,
        .retrieval_timestamp = retrieval_timestamp};
}

void WeatherCore::handleRefreshSuccess(unsigned long intervalMs)
{
    _consecutiveFailures = 0;

    if (intervalMs > 0)
    {
        _scheduler->addIntervalSample(intervalMs);
    }

    // Schedule next refresh
    unsigned long dataUtcTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      weatherData.data_timestamp.time_since_epoch())
                                      .count();
    bool scheduled = _scheduler->scheduleNextRefresh(dataUtcTimeMs);
    if (!scheduled)
    {
        logger.info("[WeatherCore] Next refresh scheduled during night time, updates paused");
        _renderer->renderNightModeIndicator();
        _displayManager->refreshDisplay();
    }
}

void WeatherCore::handleRefreshFailure()
{
    _consecutiveFailures++;

    logger.warning("[WeatherCore] Consecutive failures: %d/%d",
                   _consecutiveFailures, UpdateSchedule::MAX_CONSECUTIVE_FAILURES);

    if (_consecutiveFailures >= UpdateSchedule::MAX_CONSECUTIVE_FAILURES)
    {
        logger.error("[WeatherCore] Max failures reached! Clearing display and stopping updates.");
        _displayManager->clearDisplay();
        _updateLoopStopped = true;
    }
    else
    {
        // Schedule retry with minimum interval
        unsigned long retryDelay = UpdateSchedule::MIN_REFRESH_INTERVAL_MS;
        _scheduler->setNextScheduledRefreshMillis(millis() + retryDelay);

        logger.info("[WeatherCore] Retry scheduled in %lu seconds", retryDelay / 1000);
    }
}
