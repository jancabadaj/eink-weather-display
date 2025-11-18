
#pragma once

#include <memory>

#include "../drawUtils.h"
#include "../fonts/font58-mono.h"
#include "../shapes/degree.h"
#include "../shapes/temperature.h"

class TemperatureComponent
{
public:
    TemperatureComponent(uint8_t *imageData, int startX, int startY)
        : _imageData(imageData), _startX(startX), _startY(startY) {}

    void render(float temperatureCelsius)
    {
        char tempStr[32];

        Shape &font = Font58_Roboto_BoldCondensed_Monospace;

        DrawUtils::drawIcon(_imageData, _startX, _startY, &TemperatureIcon, Black);
        snprintf(tempStr, sizeof(tempStr), "%.1f", temperatureCelsius);
        DrawUtils::drawString(_imageData, _startX + 55, _startY + 10, tempStr, &font, Black);
        DrawUtils::drawIcon(_imageData, _startX + 235, _startY + 10, &DegreeIcon, Black);
        DrawUtils::drawChar(_imageData, _startX + 250, _startY + 10, 'C', &font, Black);
    }

private:
    uint8_t *_imageData;
    int _startX;
    int _startY;
};