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
#include "weatherCore.h"
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
    WeatherCore weatherCore{clock, http, auth, screens, panel, scheduler, serverClock};
    WebServer webServer{clock, network, weatherCore, scheduler, panel, auth, settings};

    Web::Response get(const std::string &path, std::map<std::string, std::string> query = {})
    {
        Web::Request request;
        request.method = Web::Method::Get;
        request.path = path;
        request.query = std::move(query);
        return webServer.handle(request);
    }
};
