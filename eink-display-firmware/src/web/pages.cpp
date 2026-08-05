#include "pages.h"

#include "webUiTemplate.h"

namespace
{
    void replaceAll(std::string &content, const std::string &placeholder, const std::string &value)
    {
        for (size_t at = content.find(placeholder); at != std::string::npos;
             at = content.find(placeholder, at + value.size()))
        {
            content.replace(at, placeholder.size(), value);
        }
    }
} // namespace

namespace Pages
{
    std::string formatDuration(uint64_t ms)
    {
        uint64_t s = ms / 1000;
        uint64_t m = s / 60;
        s %= 60;
        uint64_t h = m / 60;
        m %= 60;
        const uint64_t d = h / 24;
        h %= 24;

        if (d > 0)
        {
            return std::to_string(d) + "d " + std::to_string(h) + "h " + std::to_string(m) + "m " + std::to_string(s) + "s";
        }
        if (h > 0)
        {
            return std::to_string(h) + "h " + std::to_string(m) + "m " + std::to_string(s) + "s";
        }
        if (m > 0)
        {
            return std::to_string(m) + "m " + std::to_string(s) + "s";
        }
        return std::to_string(s) + "s";
    }

    std::string renderHome(const StatusSnapshot &status)
    {
        std::string tokenExpiresHuman;
        const int64_t tokenRemainingMs = (int64_t)(status.tokenExpiryUptimeMs - status.uptimeMs);
        if (!status.loggedIn)
        {
            tokenExpiresHuman = "N/A";
        }
        else if (tokenRemainingMs <= 0)
        {
            tokenExpiresHuman = "Expiring...";
        }
        else
        {
            tokenExpiresHuman = formatDuration((uint64_t)tokenRemainingMs);
        }

        std::string nextRefreshHuman;
        if (status.updatesStopped)
        {
            nextRefreshHuman = "Stopped";
        }
        else
        {
            const int64_t untilRefreshMs = (int64_t)(status.nextRefreshUptimeMs - status.uptimeMs);
            nextRefreshHuman = untilRefreshMs <= 0 ? "Now" : "in " + formatDuration((uint64_t)untilRefreshMs);
        }

        const std::string stoppedWarning =
            status.updatesStopped
                ? "<p class=\"tag-warn\">&#9888; Auto-update stopped after consecutive failures. Use &ldquo;Restart auto-update&rdquo; to resume.</p>"
                : "";

        const std::string nightTag = status.nightOverridden
                                         ? "<span class=\"tag-override\">(override)</span>"
                                         : "<span class=\"tag-default\">(default)</span>";
        const std::string nightDisplay = std::to_string(status.nightStartHour) + ":00&ndash;" +
                                         std::to_string(status.nightEndHour) + ":00 UTC " + nightTag;

        std::string html = WEB_UI_HTML;
        replaceAll(html, "{{IP}}", status.localAddress);
        replaceAll(html, "{{UPTIME_HUMAN}}", formatDuration(status.uptimeMs));
        replaceAll(html, "{{MILLIS}}", std::to_string(status.uptimeMs));
        replaceAll(html, "{{LOGGED_IN}}", status.loggedIn ? "Yes" : "No");
        replaceAll(html, "{{TOKEN_EXPIRES_HUMAN}}", tokenExpiresHuman);
        replaceAll(html, "{{TOKEN_EXPIRY_MS}}", std::to_string(status.tokenExpiryUptimeMs));
        replaceAll(html, "{{ACCESS_TOKEN}}", status.accessToken.empty() ? "(none)" : status.accessToken);
        replaceAll(html, "{{REFRESH_TOKEN}}", status.refreshToken.empty() ? "(none)" : status.refreshToken);
        replaceAll(html, "{{UPDATES_STOPPED}}", stoppedWarning);
        replaceAll(html, "{{NEXT_REFRESH_HUMAN}}", nextRefreshHuman);
        replaceAll(html, "{{NIGHT_DISPLAY}}", nightDisplay);
        replaceAll(html, "{{NIGHT_START_VAL}}", std::to_string(status.nightStartHour));
        replaceAll(html, "{{NIGHT_END_VAL}}", std::to_string(status.nightEndHour));
        replaceAll(html, "{{LOGIN_URL}}", status.loginUrl);
        return html;
    }
} // namespace Pages
