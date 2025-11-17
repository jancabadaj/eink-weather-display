#pragma once

class Auth
{
public:
    String const &getAccessToken() const { return _accessToken; }
    const bool isLoggedIn() const;

    bool login(const String &code);
    bool refreshTokenIfNeeded();

private:
    bool exchangeToken(const String &requestBody);

    bool _loggedIn;
    String _accessToken;
    String _refreshToken;
    unsigned long _tokenExpirationTimeMs;

    friend class WebServer;
};
