#pragma once

#include <string>

#include "../domain/pressureHistory.h"
#include "../domain/weatherData.h"

// Netatmo response decoding
namespace NetatmoParse
{
    struct StationData
    {
        WeatherData weather{};
        std::string deviceId;
    };

    struct TokenSet
    {
        std::string accessToken;
        std::string refreshToken;
        long expiresInSeconds = 0;
    };

    // Each returns false and leaves its out-parameter untouched when the payload is malformed or missing the fields it needs
    bool parseStationData(const std::string &payload, StationData &out);
    bool parseMeasure(const std::string &payload, PressureHistory &out);
    bool parseTokenResponse(const std::string &payload, TokenSet &out);
} // namespace NetatmoParse
