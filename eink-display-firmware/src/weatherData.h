#pragma once

#include <string>
#include <chrono>

struct WeatherDataInternal
{
    float temperature;
    int humidity;
    float pressure;
    int noise;
    int co2;
};

struct WeatherDataExternal
{
    float temperature;
    int humidity;
};

struct WeatherData
{
    WeatherDataInternal internal;
    WeatherDataExternal external;
    std::chrono::system_clock::time_point timestamp;
    // TODO: Add netatmo module timestamp + server timestamp
    //(to see the data point of measure and retrieval)
    // maybe later can be synchronized to try to retrieve fresh data
};
