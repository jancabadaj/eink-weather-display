#pragma once

#include <cstdint>
#include <string>

#include "../platform/clock.h"
#include "../platform/storage.h"

class Auth
{
public:
    Auth(Clock &clock, Storage &storage) : _clock(clock), _storage(storage) {}

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
    Storage &_storage;
    std::string _accessToken;
    std::string _refreshToken;
    std::string _state;
    uint64_t _tokenExpirationTimeMs = 0;

    friend class WebServer;
};
