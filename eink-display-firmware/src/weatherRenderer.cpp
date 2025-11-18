#include "definitions.h"
#include "weatherRenderer.h"
#include "draw/drawUtils.h"
#include "draw/fonts/font24.h"
#include "draw/shapes/pressure.h"
#include "draw/shapes/noise.h"
#include "draw/components/temperatureComponent.hpp"
#include "draw/components/humidityComponent.hpp"
#include "draw/components/co2Component.hpp"

void WeatherRenderer::renderWeather(const WeatherData &data)
{
    Shape &font24 = Font24Mono;

    DrawUtils::clearImage(_imageData);

    DrawUtils::drawLine(_imageData, IMAGE_WIDTH / 2, IMAGE_WIDTH / 2, 20, IMAGE_HEIGHT - 100, Black);
    DrawUtils::drawLine(_imageData, 20, IMAGE_WIDTH - 20, IMAGE_HEIGHT - 100, IMAGE_HEIGHT - 100, Black);

    DrawUtils::drawString(_imageData, 30, 20, "Dnu", &font24, Black);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 30, 20, "Von", &font24, Black);

    // Internal
    char tempStr[32];

    // Temperature
    TemperatureComponent tempComp(_imageData, 75, 75);
    tempComp.render(data.internal.temperature);

    // Humidity
    HumidityComponent humComp(_imageData, 65, 150);
    humComp.render(data.internal.humidity);

    // Pressure
    DrawUtils::drawShape(_imageData, 30, 315, &PressureIcon, Black);
    DrawUtils::drawString(_imageData, 60, 320, "Tlak   : ", &font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f mbar", data.internal.pressure);
    DrawUtils::drawString(_imageData, 210, 320, tempStr, &font24, Black);

    // Noise
    DrawUtils::drawShape(_imageData, 30, 345, &NoiseIcon, Black);
    DrawUtils::drawString(_imageData, 60, 350, "Hluk   : ", &font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%d dB", data.internal.noise);
    DrawUtils::drawString(_imageData, 210, 350, tempStr, &font24, Black);

    // CO2
    CO2Component co2Comp(_imageData, 65, 240);
    co2Comp.render(data.internal.co2);

    // External
    // Temperature
    TemperatureComponent extTempComp(_imageData, IMAGE_WIDTH / 2 + 40, 75);
    extTempComp.render(data.external.temperature);

    // Humidity
    HumidityComponent extHumComp(_imageData, IMAGE_WIDTH / 2 + 30, 150);
    extHumComp.render(data.external.humidity);

    // Timestamp
    auto timepoint = std::chrono::system_clock::time_point(data.data_timestamp);
    std::time_t timestamp = std::chrono::system_clock::to_time_t(timepoint);
    std::tm *tm = std::localtime(&timestamp);
    char timeStr[64];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 - 180, IMAGE_HEIGHT - 50, timeStr, &font24, Black);
}

void WeatherRenderer::renderNightModeIndicator()
{
    Shape &font = Font24Mono;

    DrawUtils::clearImage(_imageData);

    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 - 100, IMAGE_HEIGHT - 80, "Dobru noc", &font, Black);
}

void WeatherRenderer::renderNetworkError()
{
    Shape &font = Font24Mono;

    DrawUtils::clearImage(_imageData);

    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 - 150, IMAGE_HEIGHT - 80, "Chyba", &font, Black);
}