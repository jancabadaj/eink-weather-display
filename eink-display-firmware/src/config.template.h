#pragma once

namespace config
{
    // WiFi credentials
    inline const char *wifiSsid = "wifi";
    inline const char *wifiPassword = "pass";

    // Netatmo API credentials
    inline const char *apiClientId = "clientId";
    inline const char *apiClientSecret = "clientSecret";

    // Google Sheets logging (optional - leave empty for Serial-only logging)
    // Get deployment ID from your Google Apps Script deployment URL
    // Create a custom API key for authentication
    inline const char *logDeploymentId = "";
    inline const char *logApiKey = "";
}
