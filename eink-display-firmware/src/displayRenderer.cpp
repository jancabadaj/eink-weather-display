#include "displayRenderer.h"

#include "EPD.h"
#include "GUI_Paint.h"

UBYTE *imageData; /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */

void DisplayRenderer::init()
{
  DEV_Module_Init();

  // Create a new image cache
  UWORD imageSize = ((EPD_7IN5_V2_WIDTH % 8 == 0) ? (EPD_7IN5_V2_WIDTH / 8) : (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
  if ((imageData = (UBYTE *)malloc(imageSize)) == NULL)
  {
    Serial.println("Failed to apply for black memory...");
    while (1)
      ;
  }
  Paint_NewImage(imageData, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);
  Paint_SelectImage(imageData);
}

// TODO: Figure out better way to init + sleep (smart pointers? init when creating and sleep when leaving scope)
void DisplayRenderer::refreshDisplay()
{
  EPD_7IN5_V2_Init();
  EPD_7IN5_V2_Display(imageData);
  EPD_7IN5_V2_Sleep();
}

void DisplayRenderer::clearDisplay()
{
  EPD_7IN5_V2_Init();
  EPD_7IN5_V2_Clear();
  EPD_7IN5_V2_Sleep();
}
