#include <WiFi.h>
#include <Arduino.h>

#include <cctype>
#include <cstdlib>
#include <string>

#include "webServer.h"
#include "webUiTemplate.h"

static WiFiServer server(80); // Web server port number
static std::string header;    // Variable to store the HTTP request

// Timeout handling
static uint64_t previousTime = 0;
static const long timeoutTime = 2000;

static void replaceAll(std::string &content, const std::string &placeholder, const std::string &value)
{
    for (size_t at = content.find(placeholder); at != std::string::npos; at = content.find(placeholder, at + value.size()))
    {
        content.replace(at, placeholder.size(), value);
    }
}

void WebServer::init()
{
    server.begin();
    Serial.println("[WebServer] HTTP server started");
}

void WebServer::loop()
{
    WiFiClient client = server.available();
    if (!client)
        return;

    previousTime = _clock.uptimeMs();
    std::string currentLine;

    while (client.connected() && _clock.uptimeMs() - previousTime <= timeoutTime)
    {
        if (!client.available())
            continue;

        char c = client.read();
        header += c;

        if (c == '\n')
        {
            // if the current line is blank => two newline characters in a row, which means the end of the client HTTP request
            if (currentLine.empty())
            {
                bool shouldRedirect = handleRequest(client);

                if (shouldRedirect)
                {
                    client.println("HTTP/1.1 303 See Other");
                    client.println("Location: /");
                    client.println("Connection: close");
                    client.println();
                }
                else
                {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type: text/html");
                    client.println("Connection: close");
                    client.println();
                    sendHomePage(client);
                    client.println();
                }
                break;
            }
            else
            {
                currentLine.clear();
            }
        }
        else if (c != '\r')
        {
            currentLine += c;
        }
    }

    header = "";
    client.stop();
}

bool WebServer::handleRequest(WiFiClient &client)
{
    if (header.find("GET /display/restart") != std::string::npos)
    {
        Serial.println("[WebServer] Restart auto updates requested");
        _weatherCore.restartUpdateLoop();
        return true;
    }

    if (header.find("GET /display/clear") != std::string::npos)
    {
        Serial.println("[WebServer] Clear display requested");
        _display.clear();
        return true;
    }

    if (header.find("GET /data/get") != std::string::npos)
    {
        Serial.println("[WebServer] Manual data refresh requested");
        _weatherCore.reloadData();
        return true;
    }

    if (header.find("GET /config/reset") != std::string::npos)
    {
        Serial.println("[WebServer] Config reset requested");
        _configOverrides.resetAll();
        _weatherCore.restartUpdateLoop(); // apply immediately
        return true;
    }

    if (header.find("GET /config/set") != std::string::npos)
    {
        Serial.println("[WebServer] Config override requested");

        const std::string nightStartStr = parseQueryParam(header, "night_start");
        const std::string nightEndStr = parseQueryParam(header, "night_end");

        if (isValidInt(nightStartStr))
        {
            const long v = strtol(nightStartStr.c_str(), nullptr, 10);
            if (v >= 0 && v <= 23)
                _configOverrides.setNightStartHour(static_cast<int>(v));
        }
        if (isValidInt(nightEndStr))
        {
            const long v = strtol(nightEndStr.c_str(), nullptr, 10);
            if (v >= 0 && v <= 23)
                _configOverrides.setNightEndHour(static_cast<int>(v));
        }

        _weatherCore.restartUpdateLoop(); // apply immediately
        return true;
    }

    // OAuth callback — state + code arrive as query params on the root path
    const std::string callbackState = parseQueryParam(header, "state");
    const std::string callbackCode = parseQueryParam(header, "code");
    if (!callbackState.empty() && !callbackCode.empty())
    {
        Serial.println("[WebServer] Authentication callback received");
        _auth.handleCallback(callbackState, callbackCode);
        return true;
    }

    return false; // No special action, show home page
}

void WebServer::sendHomePage(WiFiClient &client)
{
    const uint64_t now = _clock.uptimeMs();

    // Uptime
    const std::string uptime = formatDuration(now);

    // Token expiry
    std::string tokenExpiresHuman;
    const int64_t tokenRemainingMs = (int64_t)(_auth._tokenExpirationTimeMs - now);
    if (!_auth.isLoggedIn())
        tokenExpiresHuman = "N/A";
    else if (tokenRemainingMs <= 0)
        tokenExpiresHuman = "Expiring...";
    else
        tokenExpiresHuman = formatDuration((uint64_t)tokenRemainingMs);

    // Next refresh
    std::string nextRefreshHuman;
    if (_weatherCore.isUpdateLoopStopped())
    {
        nextRefreshHuman = "Stopped";
    }
    else
    {
        const int64_t untilRefreshMs = (int64_t)(_scheduler.getNextScheduledRefreshMillis() - now);
        nextRefreshHuman = untilRefreshMs <= 0 ? "Now" : "in " + formatDuration((uint64_t)untilRefreshMs);
    }

    // Stopped warning
    const std::string stoppedWarning = _weatherCore.isUpdateLoopStopped()
                                           ? "<p class=\"tag-warn\">&#9888; Auto-update stopped after consecutive failures. Use &ldquo;Restart auto-update&rdquo; to resume.</p>"
                                           : "";

    // Night hours
    int ns = _configOverrides.getNightStartHour();
    int ne = _configOverrides.getNightEndHour();
    const std::string nightTag = _configOverrides.hasNightOverride()
                                     ? "<span class=\"tag-override\">(override)</span>"
                                     : "<span class=\"tag-default\">(default)</span>";
    const std::string nightDisplay = std::to_string(ns) + ":00&ndash;" + std::to_string(ne) + ":00 UTC " + nightTag;

    // Login URL (for button link)
    const std::string loginUrl = _auth.getLoginUrl();

    // Assemble template
    std::string html = WEB_UI_HTML;
    replaceAll(html, "{{IP}}", _network.localAddress());
    replaceAll(html, "{{UPTIME_HUMAN}}", uptime);
    replaceAll(html, "{{MILLIS}}", std::to_string(now));
    replaceAll(html, "{{LOGGED_IN}}", _auth.isLoggedIn() ? "Yes" : "No");
    replaceAll(html, "{{TOKEN_EXPIRES_HUMAN}}", tokenExpiresHuman);
    replaceAll(html, "{{TOKEN_EXPIRY_MS}}", std::to_string(_auth._tokenExpirationTimeMs));
    replaceAll(html, "{{ACCESS_TOKEN}}", _auth._accessToken.empty() ? "(none)" : _auth._accessToken);
    replaceAll(html, "{{REFRESH_TOKEN}}", _auth._refreshToken.empty() ? "(none)" : _auth._refreshToken);
    replaceAll(html, "{{UPDATES_STOPPED}}", stoppedWarning);
    replaceAll(html, "{{NEXT_REFRESH_HUMAN}}", nextRefreshHuman);
    replaceAll(html, "{{NIGHT_DISPLAY}}", nightDisplay);
    replaceAll(html, "{{NIGHT_START_VAL}}", std::to_string(ns));
    replaceAll(html, "{{NIGHT_END_VAL}}", std::to_string(ne));
    replaceAll(html, "{{LOGIN_URL}}", loginUrl);

    client.print(html.c_str());
}

std::string WebServer::formatDuration(uint64_t ms)
{
    uint64_t s = ms / 1000;
    uint64_t m = s / 60;
    s %= 60;
    uint64_t h = m / 60;
    m %= 60;
    uint64_t d = h / 24;
    h %= 24;

    if (d > 0)
        return std::to_string(d) + "d " + std::to_string(h) + "h " + std::to_string(m) + "m " + std::to_string(s) + "s";
    if (h > 0)
        return std::to_string(h) + "h " + std::to_string(m) + "m " + std::to_string(s) + "s";
    if (m > 0)
        return std::to_string(m) + "m " + std::to_string(s) + "s";
    return std::to_string(s) + "s";
}

std::string WebServer::parseQueryParam(const std::string &header, const std::string &key)
{
    const std::string search = key + "=";
    size_t start = header.find(search);
    if (start == std::string::npos)
        return "";
    start += search.length();

    // Parameter ends at '&', ' ' (end of URL), or end of string
    size_t end = header.length();
    const size_t amp = header.find('&', start);
    const size_t sp = header.find(' ', start);
    if (amp != std::string::npos && amp < end)
        end = amp;
    if (sp != std::string::npos && sp < end)
        end = sp;

    return header.substr(start, end - start);
}

bool WebServer::isValidInt(const std::string &s)
{
    if (s.empty())
        return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start == s.length())
        return false;
    for (size_t i = start; i < s.length(); i++)
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    return true;
}
