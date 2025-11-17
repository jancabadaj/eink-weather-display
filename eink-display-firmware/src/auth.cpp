#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "auth.h"
#include "config.h"
#include "definitions.h"
#include "logger.h"

const bool Auth::isLoggedIn() const
{
    return _loggedIn;
}

bool Auth::login(const String &code)
{
    logger.info("[Auth] Logging in");

    String requestBody = String("") +
                         "grant_type=authorization_code" + "&" +
                         "client_id=" + String(config::apiClientId) + "&" +
                         "client_secret=" + String(config::apiClientSecret) + "&" +
                         "code=" + code + "&" +
                         "redirect_uri=http://" + WiFi.localIP().toString() + "&" +
                         "scope=read_station";

    return exchangeToken(requestBody);
}

bool Auth::refreshTokenIfNeeded()
{
    // Refresh if logged in and token expires within 60 seconds
    if (_loggedIn && (millis() + 60000) > _tokenExpirationTimeMs)
    {
        logger.info("[Auth] Refreshing token (expiration at %lu, current time %lu)", _tokenExpirationTimeMs, millis());

        String requestBody = String("") +
                             "grant_type=refresh_token" + "&" +
                             "client_id=" + String(config::apiClientId) + "&" +
                             "client_secret=" + String(config::apiClientSecret) + "&" +
                             "refresh_token=" + _refreshToken;

        return exchangeToken(requestBody);
    }

    return true;
}

bool Auth::exchangeToken(const String &requestBody)
{
    HTTPClient http;
    http.begin(NETATMO_SERVER_AUTH);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    logger.debug("[Auth] Request body: %s", requestBody.c_str());

    // Send HTTP POST request
    int httpResponseCode = http.POST(requestBody);
    if (httpResponseCode > 0)
    {
        String payload = http.getString();
        logger.info("[Auth] HTTP Response code: %d", httpResponseCode);
        logger.debug("[Auth] Response payload: %s", payload.c_str());

        StaticJsonDocument<384> doc;

        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            logger.error("[Auth] deserializeJson() failed: %s", error.f_str());
            http.end();
            return false;
        }

        const char *access_token = doc["access_token"];
        const char *refresh_token = doc["refresh_token"];
        long expires_in = doc["expires_in"];

        _accessToken = String(access_token);
        _refreshToken = String(refresh_token);
        _tokenExpirationTimeMs = millis() + expires_in * 1000; // expires_in is in seconds

        http.end();
        _loggedIn = true;
        logger.info("[Auth] Token exchange successful");
        return true;
    }
    else
    {
        logger.error("[Auth] HTTP error %d: %s", httpResponseCode, http.getString().c_str());
        http.end();
        _loggedIn = false;
        return false;
    }
}
