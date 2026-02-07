#pragma once

namespace Config::Secret
{
    // WiFi credentials
    constexpr const char *wifiSsid = "wifi";
    constexpr const char *wifiPassword = "pass";

    // Netatmo API credentials
    constexpr const char *apiClientId = "clientId";
    constexpr const char *apiClientSecret = "clientSecret";

    // Google Sheets logging (optional - leave empty for Serial-only logging)
    // Get deployment ID from your Google Apps Script deployment URL
    // Create a custom API key for authentication
    constexpr const char *logDeploymentId = "";
    constexpr const char *logApiKey = "";
}
