#include "definitions.h"
#include "weatherRenderer.h"
#include "draw/drawUtils.h"
#include "draw/font24.h"

void WeatherRenderer::renderWeather(const WeatherData &data)
{
    Font &font = Font24Mono;

    DrawUtils::clearImage(_imageData);

    DrawUtils::drawLine(_imageData, IMAGE_WIDTH / 2, IMAGE_WIDTH / 2, 20, IMAGE_HEIGHT - 100, Black);
    DrawUtils::drawLine(_imageData, 20, IMAGE_WIDTH - 20, IMAGE_HEIGHT - 100, IMAGE_HEIGHT - 100, Black);

    DrawUtils::drawString(_imageData, 30, 20, "Dnu", &font, Black);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 30, 20, "Von", &font, Black);

    // Internal
    char tempStr[32];
    DrawUtils::drawString(_imageData, 30, 100, "Teplota: ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f C", data.internal.temperature);
    DrawUtils::drawString(_imageData, 180, 100, tempStr, &font, Black);

    DrawUtils::drawString(_imageData, 30, 150, "Vlhkost: ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%d %%", data.internal.humidity);
    DrawUtils::drawString(_imageData, 180, 150, tempStr, &font, Black);

    DrawUtils::drawString(_imageData, 30, 200, "Tlak   : ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f mbar", data.internal.pressure);
    DrawUtils::drawString(_imageData, 180, 200, tempStr, &font, Black);

    DrawUtils::drawString(_imageData, 30, 250, "Hluk   : ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%d dB", data.internal.noise);
    DrawUtils::drawString(_imageData, 180, 250, tempStr, &font, Black);

    DrawUtils::drawString(_imageData, 30, 300, "CO2    : ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%d ppm", data.internal.co2);
    DrawUtils::drawString(_imageData, 180, 300, tempStr, &font, Black);

    // External
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 30, 100, "Teplota: ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%.1f C", data.external.temperature);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 180, 100, tempStr, &font, Black);

    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 30, 150, "Vlhkost: ", &font, Black);
    snprintf(tempStr, sizeof(tempStr), "%d %%", data.external.humidity);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 + 180, 150, tempStr, &font, Black);

    // Timestamp
    std::time_t timestamp = std::chrono::system_clock::to_time_t(data.data_timestamp);
    std::tm *tm = std::localtime(&timestamp);
    char timeStr[64];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);
    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 - 180, IMAGE_HEIGHT - 50, timeStr, &font, Black);
}

void WeatherRenderer::renderNightModeIndicator()
{
    Font &font = Font24Mono;

    DrawUtils::drawString(_imageData, IMAGE_WIDTH / 2 - 100, IMAGE_HEIGHT - 80, "Dobru noc", &font, Black);
}
