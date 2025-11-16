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
    std::chrono::milliseconds data_timestamp;
    std::chrono::milliseconds retrieval_timestamp;
};
