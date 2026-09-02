#include "webServer.h"

#include <cctype>
#include <cstdlib>

#include "../logger.h"
#include "pages.h"

namespace
{
    // Ensures the string is a number between 0 and 23
    bool parseHour(const std::string &text, int &out)
    {
        if (text.empty())
        {
            return false;
        }
        for (const char c : text)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
            {
                return false;
            }
        }

        const long value = strtol(text.c_str(), nullptr, 10);
        if (value < 0 || value > 23)
        {
            return false;
        }

        out = static_cast<int>(value);
        return true;
    }
} // namespace

StatusSnapshot WebServer::captureStatus() const
{
    const Auth::SessionStatus session = _auth.sessionStatus();

    StatusSnapshot status;
    status.localAddress = _network.localAddress();
    status.uptimeMs = _clock.uptimeMs();

    status.loggedIn = session.loggedIn;
    status.tokenExpiryUptimeMs = session.expiryUptimeMs;
    status.accessToken = session.accessToken;
    status.refreshToken = session.refreshToken;
    status.loginUrl = _auth.getLoginUrl();

    status.updatesStopped = _app.isUpdateLoopStopped();
    status.nextRefreshUptimeMs = _scheduler.getNextScheduledRefreshMillis();

    status.nightStartHour = _configOverrides.getNightStartHour();
    status.nightEndHour = _configOverrides.getNightEndHour();
    status.nightOverridden = _configOverrides.hasNightOverride();
    return status;
}

Web::Response WebServer::applyConfig(const Web::Request &request)
{
    logger.info("[WebServer] Config override requested");

    int hour = 0;
    if (parseHour(request.param("night_start"), hour))
    {
        _configOverrides.setNightStartHour(hour);
    }
    if (parseHour(request.param("night_end"), hour))
    {
        _configOverrides.setNightEndHour(hour);
    }

    return Web::Response::seeOther("/");
}

Web::Response WebServer::handle(const Web::Request &request)
{
    if (request.method != Web::Method::Get)
    {
        return Web::Response::notFound();
    }

    if (request.path == "/display/restart")
    {
        logger.info("[WebServer] Restart auto updates requested");
        _app.restartUpdateLoop();
        return Web::Response::seeOther("/");
    }

    if (request.path == "/display/clear")
    {
        logger.info("[WebServer] Clear display requested");
        _display.clear();
        return Web::Response::seeOther("/");
    }

    if (request.path == "/data/get")
    {
        logger.info("[WebServer] Manual data refresh requested");
        _app.reloadData();
        return Web::Response::seeOther("/");
    }

    if (request.path == "/config/reset")
    {
        logger.info("[WebServer] Config reset requested");
        _configOverrides.resetAll();
        return Web::Response::seeOther("/");
    }

    if (request.path == "/config/set")
    {
        return applyConfig(request);
    }

    if (request.path == "/")
    {
        // The OAuth provider redirects back here carrying state and code.
        const std::string state = request.param("state");
        const std::string code = request.param("code");
        if (!state.empty() && !code.empty())
        {
            logger.info("[WebServer] Authentication callback received");
            _auth.handleCallback(state, code);
            return Web::Response::seeOther("/");
        }

        return Web::Response::html(Pages::renderHome(captureStatus()));
    }

    return Web::Response::notFound();
}
