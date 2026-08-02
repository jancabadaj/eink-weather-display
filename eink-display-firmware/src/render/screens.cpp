#include "../config.h"
#include "screens.h"
#include "drawUtils.h"
#include "fonts/font18-prop.h"
#include "shapes/nightSky.h"
#include "shapes/networkError.h"
#include "components/temperatureComponent.hpp"
#include "components/humidityComponent.hpp"
#include "components/co2Component.hpp"
#include "components/layoutComponent.hpp"
#include "components/pressureChartComponent.hpp"

void WeatherRenderer::renderWeather(const WeatherData &data, const PressureHistory &pressureHistory)
{
    ProportionalFont &font18 = Font18_Roboto_BoldCondensed_Proportional;

    DrawUtils::clearImage(_imageData);

    // Internal
    char tempStr[32];

    // Layout
    LayoutComponent layoutComp(_imageData);
    layoutComp.render();

    // Temperature
    TemperatureComponent tempComp(_imageData, 80, 125);
    tempComp.render(data.internal.temperature);

    // Humidity
    HumidityComponent humComp(_imageData, 70, 215);
    humComp.render(data.internal.humidity);

    // CO2
    CO2Component co2Comp(_imageData, 75, 35);
    co2Comp.render(data.internal.co2);

    // External
    // Temperature
    TemperatureComponent extTempComp(_imageData, Config::Display::width / 2 + 40, 125);
    extTempComp.render(data.external.temperature);

    // Humidity
    HumidityComponent extHumComp(_imageData, Config::Display::width / 2 + 30, 215);
    extHumComp.render(data.external.humidity);

    // Pressure chart
    PressureChartComponent pressureChart(_imageData, 40, 380, 720, 98);
    pressureChart.render(pressureHistory);

    // Timestamp
    auto timepoint = std::chrono::system_clock::time_point(data.data_timestamp);
    std::time_t timestamp = std::chrono::system_clock::to_time_t(timepoint);
    std::tm *tm = std::localtime(&timestamp);
    char timeStr[64];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
    DrawUtils::drawString(_imageData, Config::Display::width / 2 + 10, 330, timeStr, &font18, Black);
}

void WeatherRenderer::renderNightModeIndicator()
{
    DrawUtils::clearImage(_imageData);
    DrawUtils::drawShape(_imageData, 0, 0, &NightSky, Black);
}

void WeatherRenderer::renderNetworkError()
{
    DrawUtils::clearImage(_imageData);
    DrawUtils::drawShape(_imageData, Config::Display::width / 2 - NetworkError.width / 2,
                         Config::Display::height / 2 - NetworkError.height / 2,
                         &NetworkError, Black);
}
