#include <WiFi.h>
#include <Arduino.h>

#include "config.h"
#include "webServer.h"
#include "webUiTemplate.h"

// API data - TODO: Move to somewhere else? - probably belongs to auth
String uniqueState = "hello_test_unique"; // TODO: State - according to doc should be arbitrary but unique string

static WiFiServer server(80); // Web server port number
static String header;         // Variable to store the HTTP request

// Timeout handling
static unsigned long previousTime = 0;
static const long timeoutTime = 2000;

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

    previousTime = millis();
    String currentLine = "";

    while (client.connected() && millis() - previousTime <= timeoutTime)
    {
        if (!client.available())
            continue;

        char c = client.read();
        header += c;

        if (c == '\n')
        {
            // if the current line is blank => two newline characters in a row, which means the end of the client HTTP request
            if (currentLine.length() == 0)
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
                currentLine = "";
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
    if (header.indexOf("GET /display/restart") >= 0)
    {
        Serial.println("[WebServer] Restart auto updates requested");
        _weatherCore->restartUpdateLoop();
        return true;
    }

    if (header.indexOf("GET /display/clear") >= 0)
    {
        Serial.println("[WebServer] Clear display requested");
        _displayManager->clearDisplay();
        return true;
    }

    if (header.indexOf("GET /data/get") >= 0)
    {
        Serial.println("[WebServer] Manual data refresh requested");
        _weatherCore->reloadData();
        return true;
    }

    if (header.indexOf("GET /config/reset") >= 0)
    {
        Serial.println("[WebServer] Config reset requested");
        _configOverrides->resetAll();
        _weatherCore->restartUpdateLoop(); // apply immediately
        return true;
    }

    if (header.indexOf("GET /config/set") >= 0)
    {
        Serial.println("[WebServer] Config override requested");

        String nightStartStr = parseQueryParam(header, "night_start");
        String nightEndStr = parseQueryParam(header, "night_end");

        if (isValidInt(nightStartStr))
        {
            int v = nightStartStr.toInt();
            if (v >= 0 && v <= 23)
                _configOverrides->setNightStartHour(v);
        }
        if (isValidInt(nightEndStr))
        {
            int v = nightEndStr.toInt();
            if (v >= 0 && v <= 23)
                _configOverrides->setNightEndHour(v);
        }

        _weatherCore->restartUpdateLoop(); // apply immediately
        return true;
    }

    // OAuth callback — state + code arrive as query params on the root path
    String search = "GET /?state=" + uniqueState + "&code=";
    int index = header.indexOf(search);
    if (index >= 0)
    {
        Serial.println("[WebServer] Authentication callback received");
        String code = header.substring(index + search.length(), header.indexOf(' ', index + search.length()));
        _auth->login(code);
        return true;
    }

    return false; // No special action, show home page
}

void WebServer::sendHomePage(WiFiClient &client)
{
    unsigned long now = millis();

    // Uptime
    String uptime = formatDuration(now);

    // Token expiry
    String tokenExpiresHuman;
    long tokenRemainingMs = (long)(_auth->_tokenExpirationTimeMs - now);
    if (!_auth->_loggedIn)
        tokenExpiresHuman = "N/A";
    else if (tokenRemainingMs <= 0)
        tokenExpiresHuman = "Expiring...";
    else
        tokenExpiresHuman = formatDuration((unsigned long)tokenRemainingMs);

    // Next refresh
    String nextRefreshHuman;
    if (_weatherCore->isUpdateLoopStopped())
    {
        nextRefreshHuman = "Stopped";
    }
    else
    {
        long untilRefreshMs = (long)(_scheduler->getNextScheduledRefreshMillis() - now);
        nextRefreshHuman = untilRefreshMs <= 0 ? "Now" : "in " + formatDuration((unsigned long)untilRefreshMs);
    }

    // Stopped warning
    String stoppedWarning = _weatherCore->isUpdateLoopStopped()
                                ? "<p class=\"tag-warn\">&#9888; Auto-update stopped after consecutive failures. Use &ldquo;Restart auto-update&rdquo; to resume.</p>"
                                : "";

    // Night hours
    int ns = _configOverrides->getNightStartHour();
    int ne = _configOverrides->getNightEndHour();
    String nightTag = _configOverrides->hasNightOverride()
                          ? "<span class=\"tag-override\">(override)</span>"
                          : "<span class=\"tag-default\">(default)</span>";
    String nightDisplay = String(ns) + ":00&ndash;" + String(ne) + ":00 UTC " + nightTag;

    // Login URL (for button link)
    String loginUrl = "https://api.netatmo.com/oauth2/authorize?client_id=" +
                      String(Config::Secret::apiClientId) +
                      "&redirect_uri=http://" + WiFi.localIP().toString() +
                      "&scope=read_station&state=" + uniqueState;

    // Assemble template
    String html = FPSTR(WEB_UI_HTML);
    html.replace("{{IP}}", WiFi.localIP().toString());
    html.replace("{{UPTIME_HUMAN}}", uptime);
    html.replace("{{MILLIS}}", String(now));
    html.replace("{{LOGGED_IN}}", _auth->_loggedIn ? "Yes" : "No");
    html.replace("{{TOKEN_EXPIRES_HUMAN}}", tokenExpiresHuman);
    html.replace("{{TOKEN_EXPIRY_MS}}", String(_auth->_tokenExpirationTimeMs));
    html.replace("{{ACCESS_TOKEN}}", _auth->_accessToken.isEmpty() ? "(none)" : _auth->_accessToken);
    html.replace("{{REFRESH_TOKEN}}", _auth->_refreshToken.isEmpty() ? "(none)" : _auth->_refreshToken);
    html.replace("{{UPDATES_STOPPED}}", stoppedWarning);
    html.replace("{{NEXT_REFRESH_HUMAN}}", nextRefreshHuman);
    html.replace("{{NIGHT_DISPLAY}}", nightDisplay);
    html.replace("{{NIGHT_START_VAL}}", String(ns));
    html.replace("{{NIGHT_END_VAL}}", String(ne));
    html.replace("{{LOGIN_URL}}", loginUrl);

    client.print(html);
}

String WebServer::formatDuration(unsigned long ms)
{
    unsigned long s = ms / 1000;
    unsigned long m = s / 60;
    s %= 60;
    unsigned long h = m / 60;
    m %= 60;

    if (h > 0)
        return String(h) + "h " + String(m) + "m " + String(s) + "s";
    if (m > 0)
        return String(m) + "m " + String(s) + "s";
    return String(s) + "s";
}

String WebServer::parseQueryParam(const String &header, const String &key)
{
    String search = key + "=";
    int start = header.indexOf(search);
    if (start < 0)
        return "";
    start += search.length();

    // Parameter ends at '&', ' ' (end of URL), or end of string
    int end = header.length();
    int amp = header.indexOf('&', start);
    int sp = header.indexOf(' ', start);
    if (amp >= 0 && amp < end)
        end = amp;
    if (sp >= 0 && sp < end)
        end = sp;

    return header.substring(start, end);
}

bool WebServer::isValidInt(const String &s)
{
    if (s.isEmpty())
        return false;
    int start = (s[0] == '-') ? 1 : 0;
    if (start == (int)s.length())
        return false;
    for (int i = start; i < (int)s.length(); i++)
        if (!isDigit(s[i]))
            return false;
    return true;
}
