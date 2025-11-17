#include "definitions.h"
#include "weatherRenderer.h"
#include "draw/drawUtils.h"
#include "draw/font24.h"
#include "draw/font58.h"
#include "draw/shapes/degree.h"
#include "draw/shapes/temperature.h"
#include "draw/shapes/humidity.h"
#include "draw/shapes/pressure.h"
#include "draw/shapes/noise.h"
#include "draw/shapes/co2.h"

void WeatherRenderer::renderWeather(const WeatherData &data)
{
    Shape &font24 = Font24Mono;
    Shape &font = Font58_Roboto_BoldCondensed;

    DrawUtils::clearImage(_imageData);

    DrawUtils::drawLine(_imageData, IMAGE_WIDTH / 2, IMAGE_WIDTH / 2, 20, IMAGE_HEIGHT - 100, Black);
    DrawUtils::drawLine(_imageData, 20, IMAGE_WIDTH - 20, IMAGE_HEIGHT - 100, IMAGE_HEIGHT - 100, Black);

    DrawUtils::drawString(_imageData, 30, 20, "Dnu", &font24, Black);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 30, 20, "Von", &font24, Black);

    // Internal
    char tempStr[32];

    // Temperature
    DrawUtils::drawIcon(_imageData, 75, 75, &TemperatureIcon, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f", data.internal.temperature);
    DrawUtils::drawString(_imageData, 130, 85, tempStr, &font, Black);
    DrawUtils::drawIcon(_imageData, 310, 85, &DegreeIcon, Black);
    DrawUtils::drawChar(_imageData, 325, 85, 'C', &font, Black);

    // Humidity
    DrawUtils::drawIcon(_imageData, 30, 145, &HumidityIcon, Black);
    DrawUtils::drawString(_imageData, 60, 150, "Vlhkost: ", &font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%d %%", data.internal.humidity);
    DrawUtils::drawString(_imageData, 210, 150, tempStr, &font24, Black);

    // Pressure
    DrawUtils::drawIcon(_imageData, 30, 195, &PressureIcon, Black);
    DrawUtils::drawString(_imageData, 60, 200, "Tlak   : ", &font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f mbar", data.internal.pressure);
    DrawUtils::drawString(_imageData, 210, 200, tempStr, &font24, Black);

    // Noise
    DrawUtils::drawIcon(_imageData, 30, 245, &NoiseIcon, Black);
    DrawUtils::drawString(_imageData, 60, 250, "Hluk   : ", &font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%d dB", data.internal.noise);
    DrawUtils::drawString(_imageData, 210, 250, tempStr, &font24, Black);

    // CO2
    DrawUtils::drawIcon(_imageData, 30, 295, &CO2Icon, Black);
    DrawUtils::drawString(_imageData, 60, 300, "CO2    : ", &font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%d ppm", data.internal.co2);
    DrawUtils::drawString(_imageData, 210, 300, tempStr, &font24, Black);

    // External
    // Temperature
    DrawUtils::drawIcon(_imageData, IMAGE_WIDTH / 2 + 30, 75, &TemperatureIcon, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f", data.external.temperature);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 90, 85, tempStr, &font, Black);
    DrawUtils::drawIcon(_imageData, IMAGE_WIDTH / 2 + 270, 85, &DegreeIcon, Black);
    DrawUtils::drawChar(_imageData, IMAGE_WIDTH / 2 + 285, 85, 'C', &font, Black);

    // Humidity
    DrawUtils::drawIcon(_imageData, IMAGE_WIDTH / 2 + 30, 145, &HumidityIcon, Black);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 60, 150, "Vlhkost: ", &font24, Black);
    snprintf(tempStr, sizeof(tempStr), "%d %%", data.external.humidity);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 210, 150, tempStr, &font24, Black);

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