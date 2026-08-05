#include "netatmoParse.h"

#include <ArduinoJson.h>

#include "../logger.h"

namespace
{
    std::chrono::milliseconds epochSecondsToMs(uint64_t seconds)
    {
        return std::chrono::milliseconds(seconds * 1000ULL);
    }
} // namespace

namespace NetatmoParse
{
    bool parseStationData(const std::string &payload, StationData &out)
    {
        JsonDocument doc;
        if (const DeserializationError error = deserializeJson(doc, payload))
        {
            logger.error("[NetatmoParse] Station data is not valid JSON: %s", error.c_str());
            return false;
        }

        // Extract device ID for usage in history API call
        JsonObject device = doc["body"]["devices"][0];
        if (device.isNull())
        {
            logger.error("[NetatmoParse] Station data contains no devices");
            return false;
        }

        StationData parsed;
        parsed.deviceId = device["_id"] | "";

        // Internal data (main station)
        JsonObject internalDash = device["dashboard_data"];
        parsed.weather.internal.temperature = internalDash["Temperature"] | 0.0f;
        parsed.weather.internal.humidity = internalDash["Humidity"] | 0;
        parsed.weather.internal.pressure = internalDash["Pressure"] | 0.0f;
        parsed.weather.internal.noise = internalDash["Noise"] | 0;
        parsed.weather.internal.co2 = internalDash["CO2"] | 0;

        // External data (first outdoor module)
        JsonObject externalDash = device["modules"][0]["dashboard_data"];
        parsed.weather.external.temperature = externalDash["Temperature"] | 0.0f;
        parsed.weather.external.humidity = externalDash["Humidity"] | 0;

        // Timestamp
        parsed.weather.data_timestamp = epochSecondsToMs(internalDash["time_utc"] | 0ULL);
        parsed.weather.retrieval_timestamp = epochSecondsToMs(doc["time_server"] | 0ULL);

        out = parsed;
        return true;
    }

    bool parseMeasure(const std::string &payload, PressureHistory &out)
    {
        JsonDocument doc;
        if (const DeserializationError error = deserializeJson(doc, payload))
        {
            logger.error("[NetatmoParse] Measure data is not valid JSON: %s", error.c_str());
            return false;
        }

        JsonObject body = doc["body"];
        if (body.isNull())
        {
            logger.error("[NetatmoParse] Measure data has no body");
            return false;
        }

        PressureHistory parsed;
        for (JsonPair entry : body)
        {
            JsonArray values = entry.value().as<JsonArray>();
            if (values.size() == 0)
            {
                continue;
            }
            parsed.addReading(strtoul(entry.key().c_str(), nullptr, 10), values[0] | 0.0f);
        }

        out = parsed;
        return true;
    }

    bool parseTokenResponse(const std::string &payload, TokenSet &out)
    {
        JsonDocument doc;
        if (const DeserializationError error = deserializeJson(doc, payload))
        {
            logger.error("[NetatmoParse] Token response is not valid JSON: %s", error.c_str());
            return false;
        }

        const char *accessToken = doc["access_token"];
        const char *refreshToken = doc["refresh_token"];
        if (accessToken == nullptr || refreshToken == nullptr)
        {
            logger.error("[NetatmoParse] Token response is missing access_token or refresh_token");
            return false;
        }

        out.accessToken = accessToken;
        out.refreshToken = refreshToken;
        out.expiresInSeconds = doc["expires_in"] | 0L;
        return true;
    }
} // namespace NetatmoParse
