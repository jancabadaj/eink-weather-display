#pragma once

#include <string>

#include "../platform/httpClient.h"
#include "auth.h"
#include "weatherProvider.h"

class NetatmoProvider : public WeatherProvider
{
public:
    NetatmoProvider(HttpClient &http, Auth &auth) : _http(http), _auth(auth) {}

    bool isAvailable() const override { return _auth.isLoggedIn(); }
    bool refreshCredentials() override { return _auth.refreshTokenIfNeeded(); }

    bool fetchCurrent(WeatherData &out) override;
    bool fetchHistory(uint64_t nowSec, PressureHistory &out) override;

private:
    HttpClient &_http;
    Auth &_auth;

    // Learned from the station response, needed to ask for pressure history
    std::string _deviceId;
};
