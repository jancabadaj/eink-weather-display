#pragma once

#include <cstdint>
#include <string>

#include "../platform/clock.h"

class Auth
{
public:
    explicit Auth(Clock &clock) : _clock(clock) {}

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

    Clock &_clock;
    std::string _accessToken;
    std::string _refreshToken;
    std::string _state;
    uint64_t _tokenExpirationTimeMs = 0;

    friend class WebServer;
};
