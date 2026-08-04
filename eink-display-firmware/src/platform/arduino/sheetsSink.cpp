#include "sheetsSink.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <cstring>

#include "../../logger.h"

SheetsSink::SheetsSink(const char *deploymentId, const char *apiKey)
{
    if (deploymentId != nullptr && apiKey != nullptr &&
        strlen(deploymentId) > 0 && strlen(apiKey) > 0)
    {
        _deploymentId = deploymentId;
        _apiKey = apiKey;
        _enabled = true;
    }
}

void SheetsSink::write(LogLevel level, const char *message)
{
    if (!_enabled || WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    HTTPClient http;
    const std::string url = "https://script.google.com/macros/s/" + _deploymentId + "/exec";
    if (!http.begin(url.c_str()))
    {
        return; // Silently fail
    }

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    std::string encodedMessage;
    encodedMessage.reserve(strlen(message) + 8);
    for (const char *c = message; *c; c++)
    {
        switch (*c)
        {
        case ' ':
            encodedMessage += '+';
            break;
        case '\n':
            encodedMessage += "%0A";
            break;
        case '\r':
            encodedMessage += "%0D";
            break;
        default:
            encodedMessage += *c;
        }
    }

    const std::string postData = "key=" + _apiKey +
                                 "&log=" + encodedMessage +
                                 "&level=" + Logger::levelToString(level);

    // Send POST request (non-blocking, fire and forget)
    http.POST(postData.c_str());
    http.end();
}
