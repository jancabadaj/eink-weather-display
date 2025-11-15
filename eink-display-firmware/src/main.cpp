#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "DEV_Config.h"

#include "definitions.h"
#include "config.h"
#include "hw/displayManager.h"
#include "auth.h"
#include "webServer.h"
#include "weatherRenderer.h"
#include "weatherCore.h"

UBYTE *imageData; /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */

std::shared_ptr<Auth> auth;
std::shared_ptr<DisplayManager> displayManager;
std::shared_ptr<WeatherRenderer> renderer;
std::shared_ptr<WeatherCore> weatherCore;
std::shared_ptr<WebServer> webServer;

void setup()
{
  // Create a new image cache
  uint16_t imageSize = ((IMAGE_WIDTH % 8 == 0) ? (IMAGE_WIDTH / 8) : (IMAGE_WIDTH / 8 + 1)) * IMAGE_HEIGHT;
  if ((imageData = (uint8_t *)malloc(imageSize)) == NULL)
  {
    Serial.println("Failed to allocate memory...");
    while (1)
      ;
  }

  // Initialize components
  auth = std::make_shared<Auth>();
  displayManager = std::make_shared<DisplayManager>(imageData);
  renderer = std::make_shared<WeatherRenderer>(imageData);
  weatherCore = std::make_shared<WeatherCore>(auth, renderer, displayManager);
  webServer = std::make_shared<WebServer>(weatherCore, auth);

  // Initialize serial and display - must be first to allocate memory for imageData
  displayManager->init();
  displayManager->clearDisplay();
  Serial.println("start");

  Serial.print("Connecting to ");
  Serial.println(config::wifiSsid);
  WiFi.begin(config::wifiSsid, config::wifiPassword);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  webServer->init();

  /*
  DEV_Delay_ms(2000);
  Serial.println("Clear...");
  clearDisplay();

  DEV_Delay_ms(5000);
  Serial.println("Draw...");

  // Paint
  Paint_Clear(WHITE);
  Paint_DrawPoint(10, 80, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
  Paint_DrawPoint(10, 90, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
  Paint_DrawPoint(10, 100, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
  Paint_DrawLine(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(70, 70, 20, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawRectangle(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawRectangle(80, 70, 130, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawCircle(45, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawCircle(105, 95, 20, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawLine(85, 95, 125, 95, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
  Paint_DrawLine(105, 75, 105, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
  Paint_DrawString_EN(10, 0, "waveshare", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(10, 20, "hello world", &Font12, WHITE, BLACK);
  Paint_DrawNum(10, 33, 123456789, &Font12, BLACK, WHITE);
  Paint_DrawNum(10, 50, 987654321, &Font16, WHITE, BLACK);
  Paint_DrawString_CN(130, 0, " 你好abc", &Font12CN, BLACK, WHITE);
  Paint_DrawString_CN(130, 20, "微雪电子", &Font24CN, WHITE, BLACK);

  refreshDisplay();
  */
}

void loop()
{
  webServer->loop();
  weatherCore->loop();

  //  Serial.print("-");
  delay(500);
}
