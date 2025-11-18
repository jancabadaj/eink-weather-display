
#pragma once

#include <memory>
#include <algorithm>

#include "../drawUtils.h"
#include "../fonts/font58-prop.h"
#include "../shapes/humidityEmpty.h"
#include "../shapes/humidityFull.h"

class HumidityComponent
{
public:
    HumidityComponent(uint8_t *imageData, int startX, int startY)
        : _imageData(imageData), _startX(startX), _startY(startY) {}

    void render(int humidityPercentage)
    {
        humidityPercentage = std::clamp(humidityPercentage, 0, 100);

        char tempStr[32];
        ProportionalFont &font = Font58_Roboto_BoldCondensed_Proportional;
        snprintf(tempStr, sizeof(tempStr), "%d%%", humidityPercentage);
        DrawUtils::drawStringProp(_imageData, _startX + 65, _startY + 12, tempStr, &font, Black);

        // Draw empty icon first
        DrawUtils::drawShape(_imageData, _startX, _startY, &HumidityEmptyIcon, Black);

        // Partially overlay with full icon based on humidity percentage
        // Scale humidity: Too low or too high is outside of filled area (droplet shape)
        const float minScale = 0.10f;
        const float maxScale = 0.80f;
        float scaledHumidity = minScale + (maxScale - minScale) * (humidityPercentage / 100.0f);

        // Create a temporary icon shape for the overlay (to draw the bottom part)
        int overlayHeight = static_cast<int>(HumidityFullIcon.height * scaledHumidity);
        Shape partialHumidityIcon = HumidityFullIcon;
        partialHumidityIcon.height = overlayHeight;
        // skip bytes from the top of the bitmap based on remaining height
        partialHumidityIcon.bitmap += (HumidityFullIcon.width / 8 + (HumidityFullIcon.width % 8 ? 1 : 0)) * (HumidityFullIcon.height - overlayHeight);
        DrawUtils::drawShape(_imageData, _startX, _startY + (HumidityFullIcon.height - overlayHeight), &partialHumidityIcon, Black);
    }

private:
    uint8_t *_imageData;
    int _startX;
    int _startY;
};