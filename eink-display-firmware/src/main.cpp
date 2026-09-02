#include <Arduino.h>
#include <memory>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include "DEV_Config.h"

#include "config.h"
#include "schedule/updateScheduler.h"
#include "platform/arduino/arduinoClock.h"
#include "platform/arduino/arduinoHttpClient.h"
#include "platform/arduino/nvsStorage.h"
#include "platform/arduino/serialSink.h"
#include "platform/arduino/sheetsSink.h"
#include "platform/arduino/epdPanel.h"
#include "platform/arduino/espRandom.h"
#include "platform/arduino/wifiNetwork.h"
#include "platform/arduino/wifiTransport.h"
#include "provider/auth.h"
#include "provider/netatmoProvider.h"
#include "web/webServer.h"
#include "render/frameBuffer.h"
#include "render/screens.h"
#include "app.h"
#include "schedule/serverClock.h"
#include "logger.h"

// Must stay a file-scope static: at 48 KB this lands in .bss, where the linker
// reserves it up front. As a local it would go on the loop task's stack, which
// is 8 KB (CONFIG_ARDUINO_LOOP_STACK_SIZE), and overflow immediately.
static FrameBuffer frame;

std::unique_ptr<ArduinoClock> systemClock;
std::unique_ptr<SerialSink> serialSink;
std::unique_ptr<SheetsSink> sheetsSink;
std::unique_ptr<ArduinoHttpClient> httpClient;
std::unique_ptr<WifiNetwork> network;
std::unique_ptr<EspRandom> randomSource;
std::unique_ptr<NvsStorage> configStorage;
std::unique_ptr<NvsStorage> authStorage;

std::unique_ptr<ConfigOverrides> configOverrides;
std::unique_ptr<Auth> auth;
std::unique_ptr<ServerClock> serverClock;
std::unique_ptr<UpdateScheduler> updateScheduler;
std::unique_ptr<EpdPanel> panel;
std::unique_ptr<Screens> renderer;
std::unique_ptr<NetatmoProvider> provider;
std::unique_ptr<App> app;
std::unique_ptr<WebServer> webServer;
std::unique_ptr<WifiTransport> transport;

void setup()
{
    // Initialize components
    systemClock = std::make_unique<ArduinoClock>();
    httpClient = std::make_unique<ArduinoHttpClient>();
    network = std::make_unique<WifiNetwork>();
    randomSource = std::make_unique<EspRandom>();
    configStorage = std::make_unique<NvsStorage>("cfg");
    authStorage = std::make_unique<NvsStorage>("auth");

    panel = std::make_unique<EpdPanel>();
    renderer = std::make_unique<Screens>(frame);

    configOverrides = std::make_unique<ConfigOverrides>(*configStorage);
    serverClock = std::make_unique<ServerClock>(*systemClock);
    updateScheduler = std::make_unique<UpdateScheduler>(*systemClock, *serverClock, *configOverrides);

    auth = std::make_unique<Auth>(*systemClock, *httpClient, *authStorage, *network, *randomSource,
                                  Credentials{Config::Secret::apiClientId, Config::Secret::apiClientSecret});

    provider = std::make_unique<NetatmoProvider>(*httpClient, *auth);
    app = std::make_unique<App>(*systemClock, *provider, *renderer, *panel, *updateScheduler,
                                *serverClock);

    webServer = std::make_unique<WebServer>(*systemClock, *network, *app, *updateScheduler,
                                            *panel, *auth, *configOverrides);
    transport = std::make_unique<WifiTransport>(*systemClock, *webServer);

    // Must be first, calls DEV_Module_Init(), which runs Serial.begin(), and brings up the SPI/GPIO module
    panel->init();

    Serial.print("Connecting to ");
    Serial.println(Config::Secret::wifiSsid);
    WiFi.begin(Config::Secret::wifiSsid, Config::Secret::wifiPassword);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());

    // Initialize logging
    serialSink = std::make_unique<SerialSink>();
    sheetsSink = std::make_unique<SheetsSink>(Config::Secret::logDeploymentId, Config::Secret::logApiKey);

    logger.addSink(*serialSink, LogLevel::DEBUG);
    if (sheetsSink->enabled())
    {
        logger.addSink(*sheetsSink, Config::Log::minRemoteLevel);
        logger.info("[Logger] Google Sheets logging enabled");
    }
    else
    {
        logger.info("[Logger] Serial-only logging");
    }

    panel->clear();

    configOverrides->init();
    auth->loadTokens();
    transport->begin();

    logger.critical("System startup");
}

void loop()
{
    transport->poll();
    app->tick();

    // TODO: light sleep causes issues - random reboots
    // esp_sleep_enable_timer_wakeup(1000 * 1000); // 1 second in microseconds
    // esp_light_sleep_start();
    delay(1000);
}
