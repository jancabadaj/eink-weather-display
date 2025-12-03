#pragma once

#define IMAGE_WIDTH 800
#define IMAGE_HEIGHT 480
#define IMAGE_WIDTH_BYTE ((IMAGE_WIDTH % 8 == 0) ? (IMAGE_WIDTH / 8) : (IMAGE_WIDTH / 8 + 1))
#define IMAGE_HEIGHT_BYTE IMAGE_HEIGHT

#define NETATMO_SERVER_AUTH "https://api.netatmo.com/oauth2/token"
#define NETATMO_SERVER_DATA "https://api.netatmo.com/api/getstationsdata"

#define TOKEN_REFRESH_MARGIN_MS 60000 // Refresh token 1 minute before expiration

// TODO: Maybe create config and config.private
namespace UpdateSchedule
{
    // Define night hours (hour >= NIGHT_START_HOUR OR hour < NIGHT_END_HOUR is night)
    constexpr int NIGHT_START_HOUR_UTC = 21;
    constexpr int NIGHT_END_HOUR_UTC = 6;

    // Netatmo is refreshing data in documented intervals
    constexpr unsigned long REFRESH_INTERVAL_MS = 10 * 60 * 1000; // 10 minutes
    constexpr unsigned long INTERVAL_OFFSET_MS = 30 * 1000;       // 30 seconds offset to avoid fetching too early
    // Maximum consecutive failures before stopping updates and clearing display
    constexpr int MAX_CONSECUTIVE_FAILURES = 3;

} // namespace UpdateSchedule
