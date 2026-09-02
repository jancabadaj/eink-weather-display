// Preview harness: render one screen to a 1-bit BMP.
//
//   preview <weather|night-mode|network-error> <output.bmp>
//
// Sample data is fixed (including the timestamp) so every run is byte-identical
// and the output can be diffed against a reference after a refactor.

#include <cmath>
#include <cstring>
#include <string>

#include "config.h"
#include "render/frameBuffer.h"
#include "render/screens.h"

#include "bmp.h"

namespace
{
    // 2025-01-15 14:30:00 UTC
    constexpr long long sampleTimestampMs = 1736951400000LL;

    // One sample per chart slot, so the preview always shows the chart at the
    // density the device draws it at whatever barCount is set to.
    constexpr int sampleCount = Config::PressureChart::barCount;

    // PressureHistory buckets readings, so samples any closer together than one
    // bucket would collapse into each other and the chart would lose bars.
    constexpr unsigned long bucketSeconds =
        (Config::PressureChart::historyHours * 3600UL) / Config::PressureChart::barCount;

    // A slow swing across the whole window with a smaller ripple on top: enough
    // shape to read as a trend, enough bar-to-bar variation to spot a layout
    // problem. Stays within the range a real barometer reports.
    float samplePressure(int index)
    {
        constexpr float midpoint = 1004.0f;
        constexpr float swing = 4.0f;
        constexpr float ripple = 0.6f;
        constexpr float twoPi = 6.2831853f;

        const float phase = twoPi * (float)index / (float)sampleCount;
        return midpoint + swing * std::sin(phase) + ripple * std::sin(phase * 5.0f);
    }

    void renderSampleWeather(Screens &screens)
    {
        PressureHistory pressureHistory;

        const unsigned long nowSec = (unsigned long)(sampleTimestampMs / 1000);
        for (int i = 0; i < sampleCount; i++)
        {
            pressureHistory.addReading(nowSec - (sampleCount - 1 - i) * bucketSeconds, samplePressure(i));
        }

        WeatherData data{};
        // The reading the chart ends on, so the panel and the chart agree
        data.internal = {22.5f, 55, samplePressure(sampleCount - 1), 30, 520};
        data.external = {-20.1f, 60};
        data.data_timestamp = std::chrono::milliseconds(sampleTimestampMs);
        data.retrieval_timestamp = std::chrono::milliseconds(sampleTimestampMs);

        screens.renderWeather(data, pressureHistory);
    }
} // namespace

int main(int argc, char **argv)
{
    const std::string mode = argc > 1 ? argv[1] : "weather";
    const char *outPath = argc > 2 ? argv[2] : "frame.bmp";

    static FrameBuffer frame;
    Screens screens(frame);

    if (mode == "night-mode")
    {
        screens.renderNightModeIndicator();
    }
    else if (mode == "network-error")
    {
        screens.renderNetworkError();
    }
    else
    {
        renderSampleWeather(screens);
    }

    return bmp::write1Bit(outPath, frame.data(), Config::Display::width, Config::Display::height) ? 0 : 1;
}
