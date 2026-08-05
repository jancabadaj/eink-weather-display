#pragma once

#include "fakeClock.h"
#include "fakeHttpClient.h"
#include "fakeNetwork.h"
#include "fakeRandom.h"
#include "memStorage.h"
#include "recordingPanel.h"

#include "provider/auth.h"
#include "render/frameBuffer.h"
#include "render/screens.h"
#include "schedule/serverClock.h"
#include "schedule/updateScheduler.h"
#include "settings/configOverrides.h"
#include "app.h"
#include "provider/netatmoProvider.h"
#include "web/webServer.h"

struct AppFixture
{
    FakeClock clock;
    FakeHttpClient http;
    FakeNetwork network;
    FakeRandom random;
    MemStorage authStorage;
    MemStorage configStorage;
    RecordingPanel panel;

    FrameBuffer frame;
    Screens screens{frame};
    ConfigOverrides settings{configStorage};
    ServerClock serverClock{clock};
    UpdateScheduler scheduler{clock, serverClock, settings};
    Auth auth{clock, http, authStorage, network, random, Credentials{"client-id", "client-secret"}};
    NetatmoProvider provider{http, auth};
    App app{clock, provider, screens, panel, scheduler, serverClock};
    WebServer webServer{clock, network, app, scheduler, panel, auth, settings};

    // Puts the fixture into a logged-in state with a usable token.
    void logIn()
    {
        const std::string url = auth.getLoginUrl();
        const size_t at = url.find("&state=");
        http.queueOk(R"({"access_token":"tok","refresh_token":"ref","expires_in":10800})");
        auth.handleCallback(url.substr(at + 7), "auth-code");
    }

    Web::Response get(const std::string &path, std::map<std::string, std::string> query = {})
    {
        Web::Request request;
        request.method = Web::Method::Get;
        request.path = path;
        request.query = std::move(query);
        return webServer.handle(request);
    }
};
