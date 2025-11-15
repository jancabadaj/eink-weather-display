#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "auth.h"
#include "config.h"
#include "definitions.h"

void Auth::login(const String &code)
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

AuthData const &Auth::getAuthData() const
{
    return authData;
}
