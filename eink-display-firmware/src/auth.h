#pragma once

#include <Arduino.h>

class Auth
{
public:
    String const &getAccessToken() const { return _accessToken; }
    bool isLoggedIn() const { return !_refreshToken.isEmpty(); }

    String getLoginUrl(const String &redirectUri);
    bool handleCallback(const String &state, const String &code);
    bool refreshTokenIfNeeded();

    // Persistence between reboots
    void saveTokens();
    void loadTokens();

private:
    bool login(const String &code);
    bool exchangeToken(const String &requestBody);
    static String generateState();

    String _accessToken;
    String _refreshToken;
    String _state;
    unsigned long _tokenExpirationTimeMs = 0;

    friend class WebServer;
};
