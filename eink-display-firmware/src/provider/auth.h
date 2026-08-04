#pragma once

#include <string>

class Auth
{
public:
    const std::string &getAccessToken() const { return _accessToken; }
    bool isLoggedIn() const { return !_refreshToken.empty(); }

    std::string getLoginUrl(const std::string &redirectUri);
    bool handleCallback(const std::string &state, const std::string &code);
    bool refreshTokenIfNeeded();

    // Persistence between reboots
    void saveTokens();
    void loadTokens();

private:
    bool login(const std::string &code);
    bool exchangeToken(const std::string &requestBody);
    static std::string generateState();

    std::string _accessToken;
    std::string _refreshToken;
    std::string _state;
    unsigned long _tokenExpirationTimeMs = 0;

    friend class WebServer;
};
