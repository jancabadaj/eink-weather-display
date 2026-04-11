#pragma once

#include <Arduino.h>

class Auth
{
public:
    String const &getAccessToken() const { return _accessToken; }
    bool isLoggedIn() const { return _loggedIn; }

    bool login(const String &code);
    bool refreshTokenIfNeeded();

    // Persistence between reboots
    void saveTokens();
    void loadTokens();

private:
    bool exchangeToken(const String &requestBody);

    bool _loggedIn = false;
    String _accessToken;
    String _refreshToken;
    unsigned long _tokenExpirationTimeMs = 0;

    friend class WebServer;
};
