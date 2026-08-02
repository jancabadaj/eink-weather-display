#pragma once

#include <algorithm>
#include <cmath>

#include "../drawUtils.h"
#include "../fonts/font22-prop.h"
#include "../fonts/font18-prop.h"
#include "../../config.h"
#include "../../weatherData.h"

class PressureChartComponent
{
public:
    PressureChartComponent(uint8_t *imageData, int startX, int startY, int chartWidth, int chartHeight)
        : _imageData(imageData), _startX(startX), _startY(startY),
          _chartWidth(chartWidth), _chartHeight(chartHeight) {}

    void render(const PressureHistory &history)
    {
        if (history.count == 0)
            return;

        ProportionalFont &font = Font22_Roboto_BoldCondensed_Proportional;
        ProportionalFont &font18 = Font18_Roboto_BoldCondensed_Proportional;

        // Scale: auto-center on data, fixed range from config
        auto [minIt, maxIt] = std::minmax_element(history.values, history.values + history.count);
        float dataMid = (*minIt + *maxIt) / 2.0f;
        float effectiveRange = std::max(Config::PressureChart::scaleRange, (*maxIt - *minIt) + 2.0f);
        float scaleMin = std::floor(dataMid - effectiveRange / 2.0f);
        float scaleMax = std::ceil(dataMid + effectiveRange / 2.0f);
        effectiveRange = scaleMax - scaleMin;

        // Labels
        DrawUtils::drawString(_imageData, _startX + 4, _startY + (_chartHeight / 2) - 6, "hPa", &font18, Black);
        char labelStr[16];
        snprintf(labelStr, sizeof(labelStr), "%.0f", scaleMax);
        DrawUtils::drawString(_imageData, _startX, _startY + 8, labelStr, &font, Black);
        snprintf(labelStr, sizeof(labelStr), "%.0f", scaleMin);
        DrawUtils::drawString(_imageData, _startX, _startY + _chartHeight - 22, labelStr, &font, Black);

        // Chart area
        int labelWidth = 70;
        int chartLeft = _startX + labelWidth;
        int chartRight = _startX + _chartWidth;
        int drawWidth = chartRight - chartLeft;
        int chartBottom = _startY + _chartHeight;
        DrawUtils::drawLine(_imageData, chartLeft, chartRight, chartBottom, chartBottom, Black);

        // Bars
        int gap = 1;
        int barWidth = (drawWidth - (history.count - 1) * gap) / history.count;
        if (barWidth < 2)
        {
            barWidth = 2;
            gap = 0;
        }

        for (int i = 0; i < history.count; i++)
        {
            float normalized = std::clamp((history.values[i] - scaleMin) / effectiveRange, 0.0f, 1.0f);
            int barHeight = std::max(1, static_cast<int>(normalized * _chartHeight));
            int barX = chartLeft + i * (barWidth + gap);

            DrawUtils::drawRectangle(_imageData, barX, barX + barWidth - 1, chartBottom - barHeight, chartBottom - 1, Black, true);
        }
    }

private:
    uint8_t *_imageData;
    int _startX;
    int _startY;
    int _chartWidth;
    int _chartHeight;
};
