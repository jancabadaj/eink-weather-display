#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_random.h>

#include "auth.h"
#include "../config.h"
#include "../logger.h"

static const char *NVS_NS = "auth";
static const char *KEY_ACCESS = "access";
static const char *KEY_REFRESH = "refresh";

std::string Auth::generateState()
{
    char buf[17];
    snprintf(buf, sizeof(buf), "%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random());
    return buf;
}

std::string Auth::getLoginUrl(const std::string &redirectUri)
{
    _state = generateState();
    return std::string("https://api.netatmo.com/oauth2/authorize") +
           "?client_id=" + Config::Secret::apiClientId +
           "&redirect_uri=" + redirectUri +
           "&scope=read_station" +
           "&state=" + _state;
}

bool Auth::handleCallback(const std::string &state, const std::string &code)
{
    if (_state.empty() || state != _state)
    {
        logger.error("[Auth] OAuth state mismatch — possible CSRF");
        return false;
    }
    _state = "";
    return login(code);
}

bool Auth::login(const std::string &code)
{
    logger.info("[Auth] Logging in");

    const std::string requestBody =
        std::string("grant_type=authorization_code") +
        "&client_id=" + Config::Secret::apiClientId +
        "&client_secret=" + Config::Secret::apiClientSecret +
        "&code=" + code +
        "&redirect_uri=http://" + WiFi.localIP().toString().c_str() +
        "&scope=read_station";

    return exchangeToken(requestBody);
}

bool Auth::refreshTokenIfNeeded()
{
    // Refresh if logged in and token expires within 60 seconds
    // Use subtraction with signed cast to avoid millis() overflow at ULONG_MAX (~49.7 days)
    if (isLoggedIn() && (long)(_tokenExpirationTimeMs - millis()) <= 60000)
    {
        logger.info("[Auth] Refreshing token (expiration at %lu, current millis %lu)",
                    _tokenExpirationTimeMs, millis());

        const std::string requestBody =
            std::string("grant_type=refresh_token") +
            "&client_id=" + Config::Secret::apiClientId +
            "&client_secret=" + Config::Secret::apiClientSecret +
            "&refresh_token=" + _refreshToken;

        return exchangeToken(requestBody);
    }

    return true;
}

bool Auth::exchangeToken(const std::string &requestBody)
{
    HTTPClient http;
    http.begin(Config::Api::authUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    logger.debug("[Auth] Request body: %s", requestBody.c_str());

    const int httpResponseCode = http.POST(requestBody.c_str());
    const std::string payload = http.getString().c_str();
    http.end();

    logger.info("[Auth] HTTP Response code: %d", httpResponseCode);
    logger.debug("[Auth] Response payload: %s", payload.c_str());

    if (httpResponseCode == 200)
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            logger.error("[Auth] deserializeJson() failed: %s", error.f_str());
            return false;
        }

        const char *access_token = doc["access_token"];
        const char *refresh_token = doc["refresh_token"];
        if (!access_token || !refresh_token)
        {
            logger.error("[Auth] Token response missing access_token or refresh_token");
            return false;
        }

        long expires_in = doc["expires_in"];
        _accessToken = access_token;
        _refreshToken = refresh_token;
        _tokenExpirationTimeMs = millis() + (unsigned long)expires_in * 1000UL; // expires_in is in seconds
        logger.info("[Auth] Token exchange successful, expires in %lds", expires_in);

        saveTokens();
        return true;
    }
    else
    {
        logger.error("[Auth] Token request failed (%d): %s", httpResponseCode, payload.c_str());
        return false;
    }
}

void Auth::loadTokens()
{
    Preferences prefs;
    prefs.begin(NVS_NS, true);
    const std::string access = prefs.getString(KEY_ACCESS, "").c_str();
    const std::string refresh = prefs.getString(KEY_REFRESH, "").c_str();
    prefs.end();

    if (access.empty() || refresh.empty())
    {
        logger.info("[Auth] No stored tokens found, login required");
        return;
    }

    _accessToken = access;
    _refreshToken = refresh;
    _tokenExpirationTimeMs = millis(); // We don't know the exact expiry, so force a refresh on first use
    logger.info("[Auth] Tokens loaded from storage, will refresh on first use");
}

void Auth::saveTokens()
{
    Preferences prefs;
    prefs.begin(NVS_NS, false);
    prefs.putString(KEY_ACCESS, _accessToken.c_str());
    prefs.putString(KEY_REFRESH, _refreshToken.c_str());
    prefs.end();
}