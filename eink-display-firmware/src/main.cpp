#include <Arduino.h>
#include <memory>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include "DEV_Config.h"

#include "config.h"
#include "schedule/updateScheduler.h"
#include "platform/arduino/displayManager.h"
#include "provider/auth.h"
#include "web/webServer.h"
#include "render/frameBuffer.h"
#include "render/screens.h"
#include "weatherCore.h"
#include "schedule/serverClock.h"
#include "logger.h"

// Must stay a file-scope static: at 48 KB this lands in .bss, where the linker
// reserves it up front. As a local it would go on the loop task's stack, which
// is 8 KB (CONFIG_ARDUINO_LOOP_STACK_SIZE), and overflow immediately.
static FrameBuffer frame;

std::unique_ptr<ConfigOverrides> configOverrides;
std::unique_ptr<Auth> auth;
std::unique_ptr<ServerClock> serverClock;
std::unique_ptr<UpdateScheduler> updateScheduler;
std::unique_ptr<DisplayManager> displayManager;
std::unique_ptr<Screens> renderer;
std::unique_ptr<WeatherCore> weatherCore;
std::unique_ptr<WebServer> webServer;

void setup()
{
    // Initialize components
    configOverrides = std::make_unique<ConfigOverrides>();
    auth = std::make_unique<Auth>();
    serverClock = std::make_unique<ServerClock>();
    displayManager = std::make_unique<DisplayManager>(frame.data());
    renderer = std::make_unique<Screens>(frame.data());
    updateScheduler = std::make_unique<UpdateScheduler>(*serverClock, *configOverrides);
    weatherCore = std::make_unique<WeatherCore>(*auth, *renderer, *displayManager, *updateScheduler, *serverClock);
    webServer = std::make_unique<WebServer>(*weatherCore, *updateScheduler, *displayManager, *auth, *configOverrides);

    // Initialize serial and display
    displayManager->init();
    displayManager->clearDisplay();

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

    // Load persisted config overrides and tokens
    configOverrides->init();
    auth->loadTokens();

    // Initialize logging
    logger.init(Config::Secret::logDeploymentId, Config::Secret::logApiKey);
    logger.setLogLevel(Config::Log::minRemoteLevel);

    webServer->init();
    logger.critical("System startup");
}

void loop()
{
    webServer->loop();
    weatherCore->loop();

    // TODO: light sleep causes issues - random reboots
    // esp_sleep_enable_timer_wakeup(1000 * 1000); // 1 second in microseconds
    // esp_light_sleep_start();
    delay(1000);
}
