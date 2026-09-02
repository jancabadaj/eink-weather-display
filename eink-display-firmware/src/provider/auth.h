#pragma once

#include <cstdint>
#include <string>

#include "../platform/clock.h"
#include "../platform/httpClient.h"
#include "../platform/network.h"
#include "../platform/random.h"
#include "../platform/storage.h"

struct Credentials
{
    std::string clientId;
    std::string clientSecret;
};

class Auth
{
public:
    struct SessionStatus
    {
        bool loggedIn = false;
        uint64_t expiryUptimeMs = 0;
        std::string accessToken;
        std::string refreshToken;
    };

    SessionStatus sessionStatus() const
    {
        return {isLoggedIn(), _tokenExpirationTimeMs, _accessToken, _refreshToken};
    }

    Auth(Clock &clock, HttpClient &http, Storage &storage, Network &network, Random &random,
         Credentials credentials)
        : _clock(clock), _http(http), _storage(storage), _network(network), _random(random),
          _credentials(std::move(credentials)) {}

    const std::string &getAccessToken() const { return _accessToken; }
    bool isLoggedIn() const { return !_refreshToken.empty(); }

    std::string getLoginUrl();
    bool handleCallback(const std::string &state, const std::string &code);
    bool refreshTokenIfNeeded();

    // Persistence between reboots
    void saveTokens();
    void loadTokens();

private:
    bool login(const std::string &code);
    bool exchangeToken(const std::string &requestBody);
    std::string generateState();
    std::string getRedirectUri() const;

    Clock &_clock;
    HttpClient &_http;
    Storage &_storage;
    Network &_network;
    Random &_random;
    Credentials _credentials;

    std::string _accessToken;
    std::string _refreshToken;
    std::string _state;
    uint64_t _tokenExpirationTimeMs = 0;
};
