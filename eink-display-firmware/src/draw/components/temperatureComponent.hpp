
#pragma once

#include <memory>

#include "../drawUtils.h"
#include "../fonts/font58-prop.h"
#include "../shapes/degree.h"
#include "../shapes/temperature.h"

class TemperatureComponent
{
public:
    TemperatureComponent(uint8_t *imageData, int startX, int startY)
        : _imageData(imageData), _startX(startX), _startY(startY) {}

    void render(float temperatureCelsius)
    {
        DrawUtils::drawShape(_imageData, _startX, _startY, &TemperatureIcon, Black);

        char tempStr[32];
        ProportionalFont &font = Font58_Roboto_BoldCondensed_Proportional;
        uint16_t currentX = _startX + 55;
        snprintf(tempStr, sizeof(tempStr), "%.1f", temperatureCelsius);
        currentX += DrawUtils::drawString(_imageData, currentX, _startY + 10, tempStr, &font, Black) + 10;
        currentX += DrawUtils::drawShape(_imageData, currentX, _startY + 14, &DegreeIcon, Black);
        DrawUtils::drawChar(_imageData, currentX, _startY + 10, 'C', &font, Black);
    }

private:
    uint8_t *_imageData;
    int _startX;
    int _startY;
};