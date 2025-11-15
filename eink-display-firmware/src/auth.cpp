#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "auth.h"
#include "config.h"
#include "definitions.h"

AuthData const &Auth::getAuthData() const
{
    return authData;
}

void Auth::login(const String &code)
{
    String requestBody = String("") +
                         "grant_type=authorization_code" + "&" +
                         "client_id=" + String(config::apiClientId) + "&" +
                         "client_secret=" + String(config::apiClientSecret) + "&" +
                         "code=" + code + "&" +
                         "redirect_uri=http://" + WiFi.localIP().toString() + "&" +
                         "scope=read_station";

    exchangeToken(requestBody);
}

void Auth::refreshTokenIfNeeded()
{
    Serial.println("Refreshing token...");

    String requestBody = String("") +
                         "grant_type=refresh_token" + "&" +
                         "client_id=" + String(config::apiClientId) + "&" +
                         "client_secret=" + String(config::apiClientSecret) + "&" +
                         "refresh_token=" + authData.refreshToken;

    exchangeToken(requestBody);
}

bool Auth::exchangeToken(const String &requestBody)
{
    HTTPClient http;
    http.begin(NETATMO_SERVER_AUTH);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    Serial.print("body: ");
    Serial.println(requestBody);

    // Send HTTP POST request
    int httpResponseCode = http.POST(requestBody);
    if (httpResponseCode > 0)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);

        StaticJsonDocument<384> doc;

        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            http.end();
            return false;
        }

        const char *access_token = doc["access_token"];
        const char *refresh_token = doc["refresh_token"];
        long expires_in = doc["expires_in"];

        authData.accessToken = String(access_token);
        authData.refreshToken = String(refresh_token);
        authData.tokenExpirationTimeMs = millis() + expires_in * 1000; // expires_in is in seconds

        http.end();
        return true;
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
    }
}
