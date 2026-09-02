#include <doctest.h>

#include "config.h"
#include "fakes/appFixture.h"

namespace
{
    // The provider echoes back the state it was handed in the authorize URL.
    std::string stateFromLoginUrl(const std::string &url)
    {
        const size_t at = url.find("&state=");
        return at == std::string::npos ? std::string() : url.substr(at + 7);
    }
} // namespace

TEST_CASE("WebServer: rootRendersTheStatusPage")
{
    AppFixture app;
    app.network.address = "10.0.0.7";
    app.settings.setNightStartHour(21);

    const Web::Response response = app.get("/");

    CHECK(response.status == 200);
    CHECK(response.contentType == "text/html");
    CHECK(response.body.find("10.0.0.7") != std::string::npos);
    CHECK(response.body.find("21:00") != std::string::npos);
}

TEST_CASE("WebServer: actionEndpointsRedirectHome")
{
    AppFixture app;

    for (const char *path : {"/display/restart", "/display/clear", "/data/get", "/config/reset"})
    {
        const Web::Response response = app.get(path);
        CHECK(response.status == 303);
        CHECK(response.location == "/");
    }
}

TEST_CASE("WebServer: clearDisplayReachesThePanel")
{
    AppFixture app;

    app.get("/display/clear");

    CHECK(app.panel.clearCount == 1);
    CHECK(app.panel.presentCount == 0);
}

TEST_CASE("WebServer: manualRefreshAttemptsAFetch")
{
    AppFixture app;
    app.logIn();
    const size_t callsAfterLogin = app.http.calls.size();

    app.get("/data/get");

    CHECK(app.http.calls.size() > callsAfterLogin);
}

TEST_CASE("WebServer: configSetStoresValidHours")
{
    AppFixture app;

    app.get("/config/set", {{"night_start", "22"}, {"night_end", "6"}});

    CHECK(app.settings.getNightStartHour() == 22);
    CHECK(app.settings.getNightEndHour() == 6);
}

TEST_CASE("WebServer: configSetRejectsOutOfRangeAndNonNumericHours")
{
    AppFixture app;
    const int startBefore = app.settings.getNightStartHour();
    const int endBefore = app.settings.getNightEndHour();

    app.get("/config/set", {{"night_start", "24"}, {"night_end", "-1"}});
    CHECK(app.settings.getNightStartHour() == startBefore);
    CHECK(app.settings.getNightEndHour() == endBefore);

    app.get("/config/set", {{"night_start", "22abc"}, {"night_end", "seven"}});
    CHECK(app.settings.getNightStartHour() == startBefore);
    CHECK(app.settings.getNightEndHour() == endBefore);
}

TEST_CASE("WebServer: configSetAppliesOnlyTheParameterProvided")
{
    AppFixture app;

    app.get("/config/set", {{"night_start", "23"}});

    CHECK(app.settings.getNightStartHour() == 23);
    CHECK(app.settings.getNightEndHour() == Config::Schedule::nightEndHourUtc);
}

TEST_CASE("WebServer: configResetClearsOverrides")
{
    AppFixture app;
    app.settings.setNightStartHour(1);
    REQUIRE(app.settings.hasNightOverride());

    app.get("/config/reset");

    CHECK_FALSE(app.settings.hasNightOverride());
}

TEST_CASE("WebServer: oauthCallbackExchangesTheCode")
{
    AppFixture app;
    const std::string state = stateFromLoginUrl(app.auth.getLoginUrl());
    app.http.queueOk(R"({"access_token":"tok","refresh_token":"ref","expires_in":10800})");

    const Web::Response response = app.get("/", {{"state", state}, {"code", "abc"}});

    CHECK(response.status == 303);
    CHECK(app.auth.isLoggedIn());
    REQUIRE_FALSE(app.http.calls.empty());
    CHECK(app.http.calls.back().body.find("code=abc") != std::string::npos);
}

TEST_CASE("WebServer: rerenderingThePageKeepsAnInFlightLoginValid")
{
    AppFixture app;
    // The login link is handed out, then the status page is loaded again while
    // the user is still on the provider's consent screen.
    const std::string state = stateFromLoginUrl(app.auth.getLoginUrl());
    REQUIRE(app.get("/").status == 200);

    app.http.queueOk(R"({"access_token":"tok","refresh_token":"ref","expires_in":10800})");
    app.get("/", {{"state", state}, {"code", "abc"}});

    CHECK(app.auth.isLoggedIn());
}

TEST_CASE("WebServer: aConsumedStateIsNotAcceptedTwice")
{
    AppFixture app;
    const std::string state = stateFromLoginUrl(app.auth.getLoginUrl());

    app.http.queueOk(R"({"access_token":"tok","refresh_token":"ref","expires_in":10800})");
    app.get("/", {{"state", state}, {"code", "abc"}});
    REQUIRE(app.auth.isLoggedIn());

    // A replay of the same callback finds the state already spent, and the next
    // login link carries a different one.
    const size_t callsBefore = app.http.calls.size();
    app.get("/", {{"state", state}, {"code", "abc"}});
    CHECK(app.http.calls.size() == callsBefore);
    CHECK(stateFromLoginUrl(app.auth.getLoginUrl()) != state);
}

TEST_CASE("WebServer: rootWithOnlyOneCallbackParameterRendersThePage")
{
    AppFixture app;

    CHECK(app.get("/", {{"state", "s"}}).status == 200);
    CHECK(app.get("/", {{"code", "c"}}).status == 200);
    CHECK_FALSE(app.auth.isLoggedIn());
}

TEST_CASE("WebServer: unknownPathsAndMethodsAreNotFound")
{
    AppFixture app;

    CHECK(app.get("/nope").status == 404);

    Web::Request post;
    post.method = Web::Method::Post;
    post.path = "/";
    CHECK(app.webServer.handle(post).status == 404);
}
