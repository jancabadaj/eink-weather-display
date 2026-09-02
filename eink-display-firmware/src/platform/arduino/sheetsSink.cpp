#include "sheetsSink.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <cstring>

#include "../../logger.h"

namespace
{
    // The characters RFC 3986 guarantees never carry meaning in a URL
    bool isUnreserved(unsigned char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
               c == '-' || c == '_' || c == '.' || c == '~';
    }

    std::string formUrlEncode(const std::string &text)
    {
        static const char *hexDigits = "0123456789ABCDEF";

        std::string out;
        out.reserve(text.size() + 8);
        for (const char c : text)
        {
            // Bytes above 127 are negative as a signed char, which would make the range checks and the shift below misbehave
            const unsigned char byte = static_cast<unsigned char>(c);

            if (isUnreserved(byte))
            {
                out += c;
            }
            else if (byte == ' ')
            {
                out += '+'; // The form-encoding shorthand for a space
            }
            else
            {
                // Everything else becomes %HH: the byte written as two hex digits
                //   byte >> 4   drops the low 4 bits, leaving the high nibble
                //   byte & 0x0F masks off the high 4 bits, leaving the low nibble
                // For example '&' (0x26): 0x26 >> 4 == 0x2 -> '2', 0x26 & 0x0F == 0x6 -> '6' so it is sent as "%26" and arrives as a literal '&'
                out += '%';
                out += hexDigits[byte >> 4];
                out += hexDigits[byte & 0x0F];
            }
        }
        return out;
    }
} // namespace

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

    const std::string postData = "key=" + formUrlEncode(_apiKey) +
                                 "&log=" + formUrlEncode(message) +
                                 "&level=" + formUrlEncode(Logger::levelToString(level));

    // Send POST request (non-blocking, fire and forget)
    http.POST(postData.c_str());
    http.end();
}
