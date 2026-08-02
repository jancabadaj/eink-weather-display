
#pragma once

#include <memory>
#include <algorithm>

#include "../drawUtils.h"
#include "../fonts/font58-prop.h"
#include "../shapes/co2.h"
#include "../shapes/scaleEmpty.h"
#include "../shapes/scaleFull.h"

class CO2Component
{
public:
    CO2Component(uint8_t *imageData, int startX, int startY)
        : _imageData(imageData), _startX(startX), _startY(startY) {}

    void render(int co2Ppm)
    {
        char tempStr[32];
        ProportionalFont &font = Font58_Roboto_BoldCondensed_Proportional;
        snprintf(tempStr, sizeof(tempStr), "%d", co2Ppm);
        DrawUtils::drawString(_imageData, _startX + 150, _startY + 10, tempStr, &font, Black);

        // Draw empty scale first
        DrawUtils::drawShape(_imageData, _startX, _startY, &CO2Icon, Black);
        DrawUtils::drawShape(_imageData, _startX, _startY - 5, &ScaleEmptyIcon, Black);

        // Partially overlay with full scale based on CO2 ppm

        // Scale consists of 3 segments, because of dividers it is not linear:
        // Icon fill percentage: S1:(0.05 - 0.35) S2:(0.43 - 0.67) S3:(0.75 - 0.98)
        const float CO2_SEG1_MIN = 400.0f;
        const float CO2_SEG1_MAX = 900.0f;
        const float CO2_SEG2_MAX = 1300.0f;
        const float CO2_SEG3_MAX = 1800.0f;

        float fillPercent = 0.0f;
        if (co2Ppm <= CO2_SEG1_MAX) // Segment 1
        {
            float t = (std::clamp(static_cast<float>(co2Ppm), CO2_SEG1_MIN, CO2_SEG1_MAX) - CO2_SEG1_MIN) / (CO2_SEG1_MAX - CO2_SEG1_MIN);
            fillPercent = 0.05f + t * (0.35f - 0.05f);
        }
        else if (co2Ppm <= CO2_SEG2_MAX) // Segment 2
        {
            float t = (std::clamp(static_cast<float>(co2Ppm), CO2_SEG1_MAX, CO2_SEG2_MAX) - CO2_SEG1_MAX) / (CO2_SEG2_MAX - CO2_SEG1_MAX);
            fillPercent = 0.43f + t * (0.67f - 0.43f);
        }
        else // Segment 3
        {
            float t = (std::clamp(static_cast<float>(co2Ppm), CO2_SEG2_MAX, CO2_SEG3_MAX) - CO2_SEG2_MAX) / (CO2_SEG3_MAX - CO2_SEG2_MAX);
            fillPercent = 0.75f + t * (0.98f - 0.75f);
        }

        int overlayWidth = static_cast<int>(ScaleFullIcon.width * std::clamp(fillPercent, 0.0f, 1.0f));

        // Create a temporary icon shape for the overlay (to draw left part)
        Shape partialScaleIcon = ScaleFullIcon;
        partialScaleIcon.width = overlayWidth;
        // Bitmap needs to be re-created since we are changing width (not just height)
        int bytesPerRow = ScaleFullIcon.width / 8 + (ScaleFullIcon.width % 8 ? 1 : 0);
        int partialBytesPerRow = partialScaleIcon.width / 8 + (partialScaleIcon.width % 8 ? 1 : 0);
        static uint8_t partialBitmap[(144 / 8 + (144 % 8 ? 1 : 0)) * 64]; // Max size
        for (int row = 0; row < ScaleFullIcon.height; row++)
        {
            for (int byte = 0; byte < partialBytesPerRow; byte++)
            {
                partialBitmap[row * partialBytesPerRow + byte] = ScaleFullIcon.bitmap[row * bytesPerRow + byte];
            }
        }
        partialScaleIcon.bitmap = partialBitmap;
        DrawUtils::drawShape(_imageData, _startX, _startY - 5, &partialScaleIcon, Black);
    }

private:
    uint8_t *_imageData;
    int _startX;
    int _startY;
};