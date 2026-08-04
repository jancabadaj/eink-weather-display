#pragma once

#include "../domain/weatherData.h"
#include "../domain/pressureHistory.h"

#include "color.h"
#include "frameBuffer.h"

class Screens
{
public:
    explicit Screens(FrameBuffer &frame) : _frame(frame) {}

    void renderWeather(const WeatherData &data, const PressureHistory &pressureHistory);
    void renderNightModeIndicator();
    void renderNetworkError();

    // The currently rendered frame
    const uint8_t *pixels() const { return _frame.data(); }

private:
    FrameBuffer &_frame;
    uint8_t *_imageData = _frame.data();
};