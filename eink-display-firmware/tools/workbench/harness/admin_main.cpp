// Renders the device's admin page from a status snapshot.
//
//   admin <snapshot.json> <output.html>
//
// The snapshot arrives as JSON so the workbench can vary the device state -
// logged out, updates stopped, overrides applied - without a device.

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include <ArduinoJson.h>

#include "web/pages.h"

namespace
{
    std::string readFile(const char *path)
    {
        std::ifstream in(path);
        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    }

    StatusSnapshot snapshotFrom(const std::string &json)
    {
        StatusSnapshot status;
        status.localAddress = "192.168.1.50";
        status.uptimeMs = 93784000;
        status.loggedIn = true;
        status.accessToken = "sample-access-token";
        status.refreshToken = "sample-refresh-token";
        status.loginUrl = "https://api.netatmo.com/oauth2/authorize?client_id=demo";
        status.nightStartHour = 21;
        status.nightEndHour = 5;

        JsonDocument doc;
        const bool parsed = !deserializeJson(doc, json);

        if (parsed)
        {
            status.localAddress = doc["localAddress"] | status.localAddress;
            status.uptimeMs = doc["uptimeMs"] | status.uptimeMs;
            status.loggedIn = doc["loggedIn"] | status.loggedIn;
            status.accessToken = doc["accessToken"] | status.accessToken;
            status.refreshToken = doc["refreshToken"] | status.refreshToken;
            status.updatesStopped = doc["updatesStopped"] | status.updatesStopped;
            status.nightStartHour = doc["nightStartHour"] | status.nightStartHour;
            status.nightEndHour = doc["nightEndHour"] | status.nightEndHour;
            status.nightOverridden = doc["nightOverridden"] | status.nightOverridden;
        }

        // Held relative to uptime so the rendered durations stay meaningful.
        const uint64_t tokenExpiresInMs = parsed ? (doc["tokenExpiresInMs"] | 2700000ULL) : 2700000ULL;
        const uint64_t nextRefreshInMs = parsed ? (doc["nextRefreshInMs"] | 428000ULL) : 428000ULL;
        status.tokenExpiryUptimeMs = status.uptimeMs + tokenExpiresInMs;
        status.nextRefreshUptimeMs = status.uptimeMs + nextRefreshInMs;
        return status;
    }
} // namespace

int main(int argc, char **argv)
{
    const std::string json = argc > 1 ? readFile(argv[1]) : std::string();
    const char *outPath = argc > 2 ? argv[2] : "admin.html";

    const std::string html = Pages::renderHome(snapshotFrom(json));

    std::ofstream out(outPath, std::ios::binary);
    if (!out)
    {
        fprintf(stderr, "cannot write %s\n", outPath);
        return 1;
    }
    out << html;
    return 0;
}
