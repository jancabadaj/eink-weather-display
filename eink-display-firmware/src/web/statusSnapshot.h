#pragma once

#include <cstdint>
#include <string>

// Contains a state - everything necessary to render the admin page
struct StatusSnapshot
{
    std::string localAddress;
    uint64_t uptimeMs = 0;

    bool loggedIn = false;
    uint64_t tokenExpiryUptimeMs = 0;
    std::string accessToken;
    std::string refreshToken;
    std::string loginUrl;

    bool updatesStopped = false;
    uint64_t nextRefreshUptimeMs = 0;

    int nightStartHour = 0;
    int nightEndHour = 0;
    bool nightOverridden = false;
};
