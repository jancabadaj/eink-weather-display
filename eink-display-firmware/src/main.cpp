#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include "DEV_Config.h"

#include "config.h"
#include "updateScheduler.h"
#include "hw/displayManager.h"
#include "auth.h"
#include "webServer.h"
#include "weatherRenderer.h"
#include "weatherCore.h"
#include "serverClock.h"
#include "logger.h"

UBYTE *imageData; /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */

std::shared_ptr<Auth> auth;
std::shared_ptr<ServerClock> serverClock;
std::shared_ptr<UpdateScheduler> updateScheduler;
std::shared_ptr<DisplayManager> displayManager;
std::shared_ptr<WeatherRenderer> renderer;
std::shared_ptr<WeatherCore> weatherCore;
std::shared_ptr<WebServer> webServer;

void setup()
{
  // Create a new image cache
  uint16_t imageSize = Config::Display::widthBytes * Config::Display::heightBytes;
  if ((imageData = (uint8_t *)malloc(imageSize)) == NULL)
  {
    Serial.println("Failed to allocate memory...");
    while (1)
      ;
  }

  // Initialize components
  auth = std::make_shared<Auth>();
  serverClock = std::make_shared<ServerClock>();
  displayManager = std::make_shared<DisplayManager>(imageData);
  renderer = std::make_shared<WeatherRenderer>(imageData);
  updateScheduler = std::make_shared<UpdateScheduler>(serverClock);
  weatherCore = std::make_shared<WeatherCore>(auth, renderer, displayManager, updateScheduler, serverClock);
  webServer = std::make_shared<WebServer>(weatherCore, displayManager, auth);

  // Initialize serial and display - must be first to allocate memory for imageData
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
