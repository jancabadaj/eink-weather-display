#include "definitions.h"
#include "weatherRenderer.h"
#include "draw/drawUtils.h"
#include "draw/font24.h"
#include "draw/shapes/thermometer.h"
#include "draw/shapes/humidity.h"
#include "draw/shapes/pressure.h"
#include "draw/shapes/noise.h"
#include "draw/shapes/co2.h"

void WeatherRenderer::renderWeather(const WeatherData &data)
{
    Shape &font = Font24Mono;

    DrawUtils::clearImage(_imageData);

    DrawUtils::drawLine(_imageData, IMAGE_WIDTH / 2, IMAGE_WIDTH / 2, 20, IMAGE_HEIGHT - 100, Black);
    DrawUtils::drawLine(_imageData, 20, IMAGE_WIDTH - 20, IMAGE_HEIGHT - 100, IMAGE_HEIGHT - 100, Black);

    DrawUtils::drawString(_imageData, 30, 20, "Dnu", &font, Black);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 30, 20, "Von", &font, Black);

    // Internal
    char tempStr[32];

    // Temperature
    DrawUtils::drawIcon(_imageData, 30, 95, &ThermometerIcon, Black);
    DrawUtils::drawString(_imageData, 60, 100, "Teplota: ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f C", data.internal.temperature);
    DrawUtils::drawString(_imageData, 210, 100, tempStr, &font, Black);

    // Humidity
    DrawUtils::drawIcon(_imageData, 30, 145, &HumidityIcon, Black);
    DrawUtils::drawString(_imageData, 60, 150, "Vlhkost: ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%d %%", data.internal.humidity);
    DrawUtils::drawString(_imageData, 210, 150, tempStr, &font, Black);

    // Pressure
    DrawUtils::drawIcon(_imageData, 30, 195, &PressureIcon, Black);
    DrawUtils::drawString(_imageData, 60, 200, "Tlak   : ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f mbar", data.internal.pressure);
    DrawUtils::drawString(_imageData, 210, 200, tempStr, &font, Black);

    // Noise
    DrawUtils::drawIcon(_imageData, 30, 245, &NoiseIcon, Black);
    DrawUtils::drawString(_imageData, 60, 250, "Hluk   : ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%d dB", data.internal.noise);
    DrawUtils::drawString(_imageData, 210, 250, tempStr, &font, Black);

    // CO2
    DrawUtils::drawIcon(_imageData, 30, 295, &CO2Icon, Black);
    DrawUtils::drawString(_imageData, 60, 300, "CO2    : ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%d ppm", data.internal.co2);
    DrawUtils::drawString(_imageData, 210, 300, tempStr, &font, Black);

    // External
    // Temperature
    DrawUtils::drawIcon(_imageData, IMAGE_WIDTH / 2 + 30, 95, &ThermometerIcon, Black);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 60, 100, "Teplota: ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f C", data.external.temperature);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 210, 100, tempStr, &font, Black);

    // Humidity
    DrawUtils::drawIcon(_imageData, IMAGE_WIDTH / 2 + 30, 145, &HumidityIcon, Black);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 60, 150, "Vlhkost: ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%d %%", data.external.humidity);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 210, 150, tempStr, &font, Black);

    // Timestamp
    auto timepoint = std::chrono::system_clock::time_point(data.data_timestamp);
    std::time_t timestamp = std::chrono::system_clock::to_time_t(timepoint);
    std::tm *tm = std::localtime(&timestamp);
    char timeStr[64];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 - 180, IMAGE_HEIGHT - 50, timeStr, &font, Black);
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