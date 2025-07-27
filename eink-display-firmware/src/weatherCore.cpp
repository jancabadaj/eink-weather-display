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

void WeatherCore::authenticate(const String &code)
{
    HTTPClient http;
    http.begin(NETATMO_SERVER_AUTH);

    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Data to send with HTTP POST
    String httpRequestData = String("") +
                             "grant_type=authorization_code" + "&" +
                             "client_id=" + String(config::apiClientId) + "&" +
                             "client_secret=" + String(config::apiClientSecret) + "&" +
                             "code=" + code + "&" +
                             "redirect_uri=http://" + WiFi.localIP().toString() + "&" +
                             "scope=read_station";

    Serial.print("body: ");
    Serial.println(httpRequestData);

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);

        StaticJsonDocument<384> doc;

        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, payload);

        // Test if parsing succeeds.
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }

        // Fetch values.
        //
        // Most of the time, you can rely on the implicit casts.
        // In other case, you can do doc["time"].as<long>();
        const char *access_token = doc["access_token"];
        const char *refresh_token = doc["refresh_token"];
        long expires_in = doc["expires_in"];

        authData.accessToken = String(access_token);
        authData.refreshToken = String(refresh_token);
        authData.tokenExpirationTime = millis() + expires_in * 1000; // Expiration time is in seconds

        // TODO: before getting station data, check expiration time if still valid (maybe need to store current time when requesting token)
        //       if not valid, auto request new one with refrfesh token
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
}

AuthData const &WeatherCore::getAuthData() const
{
    return authData;
}

void WeatherCore::reloadData()
{
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
    long time_utc = internalDash["time_utc"] | 0;
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::from_time_t(time_utc);

    weatherData = {
        .internal = internal,
        .external = external,
        .timestamp = timestamp};
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
