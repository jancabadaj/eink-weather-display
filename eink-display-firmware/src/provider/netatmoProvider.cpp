#include "netatmoProvider.h"

#include "../config.h"
#include "../logger.h"
#include "netatmoParse.h"

bool NetatmoProvider::fetchCurrent(WeatherData &out)
{
    const HttpResponse response = _http.get(Config::Api::dataUrl, _auth.getAccessToken());
    if (!response.ok())
    {
        logger.error("[Netatmo] Station data request failed: %d %s", response.status,
                     response.body.c_str());
        return false;
    }

    NetatmoParse::StationData parsed;
    if (!NetatmoParse::parseStationData(response.body, parsed))
    {
        return false;
    }

    if (!parsed.deviceId.empty())
    {
        _deviceId = parsed.deviceId;
    }

    out = parsed.weather;
    return true;
}

bool NetatmoProvider::fetchHistory(uint64_t nowSec, PressureHistory &out)
{
    if (_deviceId.empty())
    {
        return false; // the station has not identified itself yet
    }

    const uint64_t dateBegin = nowSec - (Config::PressureChart::historyHours * 3600ULL);

    std::string url = Config::Api::historyUrl;
    url += "?device_id=" + _deviceId;
    url += "&date_begin=" + std::to_string(dateBegin);
    url += "&scale=1hour";
    url += "&type=pressure";
    url += "&optimize=false";
    url += "&real_time=false";

    const HttpResponse response = _http.get(url, _auth.getAccessToken());
    if (!response.ok())
    {
        logger.error("[Netatmo] Pressure history request failed: %d", response.status);
        return false;
    }

    if (!NetatmoParse::parseMeasure(response.body, out))
    {
        return false;
    }

    logger.info("[Netatmo] Pressure history loaded: %d entries", out.count);
    return true;
}
