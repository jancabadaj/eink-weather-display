#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>

#include "auth.h"
#include "config.h"
#include "logger.h"

static const char *NVS_NS = "auth";
static const char *KEY_ACCESS = "access";
static const char *KEY_REFRESH = "refresh";

bool Auth::login(const String &code)
{
    logger.info("[Auth] Logging in");

    String requestBody = String("") +
                         "grant_type=authorization_code" + "&" +
                         "client_id=" + String(Config::Secret::apiClientId) + "&" +
                         "client_secret=" + String(Config::Secret::apiClientSecret) + "&" +
                         "code=" + code + "&" +
                         "redirect_uri=http://" + WiFi.localIP().toString() + "&" +
                         "scope=read_station";

    return exchangeToken(requestBody);
}

bool Auth::refreshTokenIfNeeded()
{
    // Refresh if logged in and token expires within 60 seconds
    // Use subtraction with signed cast to avoid millis() overflow at ULONG_MAX (~49.7 days)
    if (_loggedIn && (long)(_tokenExpirationTimeMs - millis()) <= 60000)
    {
        logger.info("[Auth] Refreshing token (expiration at %lu, current millis %lu)",
                    _tokenExpirationTimeMs, millis());

        String requestBody = String("") +
                             "grant_type=refresh_token" + "&" +
                             "client_id=" + String(Config::Secret::apiClientId) + "&" +
                             "client_secret=" + String(Config::Secret::apiClientSecret) + "&" +
                             "refresh_token=" + _refreshToken;

        return exchangeToken(requestBody);
    }

    return true;
}

bool Auth::exchangeToken(const String &requestBody)
{
    HTTPClient http;
    http.begin(Config::Api::authUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    logger.debug("[Auth] Request body: %s", requestBody.c_str());

    int httpResponseCode = http.POST(requestBody);
    if (httpResponseCode > 0)
    {
        String payload = http.getString();
        logger.info("[Auth] HTTP Response code: %d", httpResponseCode);
        logger.debug("[Auth] Response payload: %s", payload.c_str());

        JsonDocument doc;
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
        _tokenExpirationTimeMs = millis() + (unsigned long)expires_in * 1000UL; // expires_in is in seconds

        http.end();
        _loggedIn = true;
        logger.info("[Auth] Token exchange successful, expires in %lds", expires_in);

        saveTokens();
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

void Auth::loadTokens()
{
    Preferences prefs;
    prefs.begin(NVS_NS, true);
    String access = prefs.getString(KEY_ACCESS, "");
    String refresh = prefs.getString(KEY_REFRESH, "");
    prefs.end();

    if (access.isEmpty() || refresh.isEmpty())
    {
        logger.info("[Auth] No stored tokens found, login required");
        return;
    }

    _accessToken = access;
    _refreshToken = refresh;
    _tokenExpirationTimeMs = millis(); // We don't know the exact expiry, so force a refresh on first use
    _loggedIn = true;
    logger.info("[Auth] Tokens loaded from storage, will refresh on first use");
}

void Auth::saveTokens()
{
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putString(KEY_ACCESS, _accessToken);
    prefs.putString(KEY_REFRESH, _refreshToken);
    prefs.end();
}