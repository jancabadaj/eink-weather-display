#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "weatherCore.h"
#include "config.h"
#include "definitions.h"

void WeatherCore::loop()
{
    // Here handle the logic of fetching weather data, processing it, and rendering it
}

void WeatherCore::reloadData()
{
    AuthData authData = _auth->getAuthData();

    HTTPClient http;
    http.begin(NETATMO_SERVER_DATA);
    http.addHeader("Authorization", "Bearer " + authData.accessToken);

    // Send HTTP GET request
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);

        parseWeatherData(payload);
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
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
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
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

// TODO: Temporary debug, delete it?
void WeatherCore::clearDisplay()
{
    _displayManager->clearDisplay();
}

// TODO: Temporary debug, delete it?
void WeatherCore::drawWeatherData()
{
    /* This function can be used to draw weather data on the display
    WeatherData data = {
        .internal = {25.0f, 60, 1013.3f, 30, 400},
        .external = {22.0f, 55},
        .timestamp = std::chrono::system_clock::now()};

    _renderer->renderWeather(data);*/
    _renderer->renderWeather(weatherData);

    _displayManager->refreshDisplay();
}
