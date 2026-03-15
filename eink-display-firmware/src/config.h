#pragma once

#include <cstdint>
#include <cstddef>

// Secret credentials - copy config.secret.template.h to config.secret.h and edit
#include "config.secret.h"

// =============================================================================
// Display
// =============================================================================

namespace Config::Display
{
    constexpr int width = 800;
    constexpr int height = 480;
    constexpr int widthBytes = (width % 8 == 0) ? (width / 8) : (width / 8 + 1);
    constexpr int heightBytes = height;
}

// =============================================================================
// Netatmo API
// =============================================================================

namespace Config::Api
{
    constexpr const char *authUrl = "https://api.netatmo.com/oauth2/token";
    constexpr const char *dataUrl = "https://api.netatmo.com/api/getstationsdata";
    constexpr const char *historyUrl = "https://api.netatmo.com/api/getmeasure";
    constexpr unsigned long tokenRefreshMarginMs = 60000; // 1 minute before expiration
}

// =============================================================================
// Update Schedule
// =============================================================================

namespace Config::Schedule
{
    // Define night hours (hour >= nightStartHourUtc OR hour < nightEndHourUtc is night)
    constexpr int nightStartHourUtc = 21;
    constexpr int nightEndHourUtc = 5;

    // Netatmo is refreshing data in documented intervals
    constexpr unsigned long refreshIntervalMs = 10 * 60 * 1000; // 10 minutes
    constexpr unsigned long intervalOffsetMs = 30 * 1000;       // 30 seconds offset to avoid fetching too early
    // Maximum consecutive failures before stopping updates and clearing display
    constexpr int maxConsecutiveFailures = 3;
}

// =============================================================================
// Pressure Chart
// =============================================================================

namespace Config::PressureChart
{
    constexpr int historyHours = 48;    // hours of history to display
    constexpr int barCount = 24;        // number of bars in the chart (downsampled from API data)
    constexpr float scaleRange = 20.0f; // fixed Y-axis range in hPa
}

// =============================================================================
// Logging
// =============================================================================

enum class LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

namespace Config::Log
{
    constexpr LogLevel minRemoteLevel = LogLevel::WARNING;
}
