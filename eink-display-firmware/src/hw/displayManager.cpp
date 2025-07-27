#include "displayManager.h"

#include "EPD.h"
#include "GUI_Paint.h"

#define IMAGE_WIDTH EPD_7IN5_V2_WIDTH
#define IMAGE_HEIGHT EPD_7IN5_V2_HEIGHT

void DisplayManager::init()
{
  DEV_Module_Init();

  // Create a new image cache
  uint16_t imageSize = ((IMAGE_WIDTH % 8 == 0) ? (IMAGE_WIDTH / 8) : (IMAGE_WIDTH / 8 + 1)) * IMAGE_HEIGHT;
  if ((_imageData = (uint8_t *)malloc(imageSize)) == NULL)
  {
    Serial.println("Failed to apply for black memory...");
    while (1)
      ;
  }
  Paint_NewImage(_imageData, IMAGE_WIDTH, IMAGE_HEIGHT, 0, WHITE);
  Paint_SelectImage(_imageData);
}

// TODO: Figure out better way to init + sleep (smart pointers? init when creating and sleep when leaving scope)
void DisplayManager::refreshDisplay()
{
  EPD_7IN5_V2_Init();
  EPD_7IN5_V2_Display(_imageData);
  EPD_7IN5_V2_Sleep();
}

void DisplayManager::clearDisplay()
{
  EPD_7IN5_V2_Init();
  EPD_7IN5_V2_Clear();
  EPD_7IN5_V2_Sleep();
}
