// Preview harness: render one screen to a 1-bit BMP.
//
//   preview <weather|night-mode|network-error> <output.bmp>
//
// Sample data is fixed (including the timestamp) so every run is byte-identical
// and the output can be diffed against a reference after a refactor.

#include <cstring>
#include <string>

#include "config.h"
#include "render/screens.h"

#include "bmp.h"

namespace
{
    // 2025-01-15 14:30:00 UTC
    constexpr long long sampleTimestampMs = 1736951400000LL;

    void renderSampleWeather(Screens &screens)
    {
        PressureHistory pressureHistory;
        const float samplePressures[] = {
            1007.5f, 1007.6f, 1007.8f, 1007.6f, 1007.9f, 1008.2f,
            1007.1f, 1006.5f, 1005.9f, 1004.9f, 1004.2f, 1003.8f,
            1003.5f, 1002.9f, 1002.5f, 1002.3f, 1001.6f, 1001.3f,
            1000.5f, 999.9f, 1000.8f, 1001.9f, 1003.3f, 1004.5f};

        const unsigned long nowSec = (unsigned long)(sampleTimestampMs / 1000);
        for (int i = 0; i < 24; i++)
        {
            pressureHistory.addReading(nowSec - (23 - i) * 3600, samplePressures[i]);
        }

        WeatherData data{};
        data.internal = {22.5f, 55, 1004.5f, 30, 520};
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

    static uint8_t frame[Config::Display::widthBytes * Config::Display::heightBytes];
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

    return bmp::write1Bit(outPath, frame, Config::Display::width, Config::Display::height) ? 0 : 1;
}
