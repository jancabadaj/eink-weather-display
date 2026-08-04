#include <ArduinoJson.h>

#include "auth.h"
#include "../config.h"
#include "../logger.h"

static const char *KEY_ACCESS = "access";
static const char *KEY_REFRESH = "refresh";

std::string Auth::generateState()
{
    char buf[17];
    snprintf(buf, sizeof(buf), "%08lx%08lx", (unsigned long)_random.next(), (unsigned long)_random.next());
    return buf;
}

std::string Auth::getRedirectUri() const
{
    return "http://" + _network.localAddress();
}

std::string Auth::getLoginUrl()
{
    _state = generateState();
    return std::string("https://api.netatmo.com/oauth2/authorize") +
           "?client_id=" + _credentials.clientId +
           "&redirect_uri=" + getRedirectUri() +
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
        "&client_id=" + _credentials.clientId +
        "&client_secret=" + _credentials.clientSecret +
        "&code=" + code +
        "&redirect_uri=" + getRedirectUri() +
        "&scope=read_station";

    return exchangeToken(requestBody);
}

bool Auth::refreshTokenIfNeeded()
{
    // Refresh if logged in and the token expires within the margin
    const uint64_t now = _clock.uptimeMs();
    if (isLoggedIn() && _tokenExpirationTimeMs <= now + Config::Api::tokenRefreshMarginMs)
    {
        logger.info("[Auth] Refreshing token (expiration at %llu, current uptime %llu)",
                    _tokenExpirationTimeMs, now);

        const std::string requestBody =
            std::string("grant_type=refresh_token") +
            "&client_id=" + _credentials.clientId +
            "&client_secret=" + _credentials.clientSecret +
            "&refresh_token=" + _refreshToken;

        return exchangeToken(requestBody);
    }

    return true;
}

bool Auth::exchangeToken(const std::string &requestBody)
{
    logger.debug("[Auth] Request body: %s", requestBody.c_str());

    const HttpResponse response = _http.postForm(Config::Api::authUrl, requestBody);
    const int httpResponseCode = response.status;
    const std::string &payload = response.body;

    logger.info("[Auth] HTTP Response code: %d", httpResponseCode);
    logger.debug("[Auth] Response payload: %s", payload.c_str());

    if (response.ok())
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            logger.error("[Auth] deserializeJson() failed: %s", error.c_str());
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
        _tokenExpirationTimeMs = _clock.uptimeMs() + (uint64_t)expires_in * 1000ULL; // expires_in is in seconds
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
    const std::string access = _storage.getString(KEY_ACCESS, "");
    const std::string refresh = _storage.getString(KEY_REFRESH, "");

    if (access.empty() || refresh.empty())
    {
        logger.info("[Auth] No stored tokens found, login required");
        return;
    }

    _accessToken = access;
    _refreshToken = refresh;
    _tokenExpirationTimeMs = _clock.uptimeMs(); // Exact expiry is unknown, so force a refresh on first use
    logger.info("[Auth] Tokens loaded from storage, will refresh on first use");
}

void Auth::saveTokens()
{
    _storage.putString(KEY_ACCESS, _accessToken);
    _storage.putString(KEY_REFRESH, _refreshToken);
}