#include "displayManager.h"

// TODO:
//#include "EPD.h"
#include "utility/EPD_7in5_V2.h"

#include "GUI_Paint.h"
#include "../../config.h"

// TODO: Paint_NewImage should be removed and DEV_Module_Init can be moved to main.cpp
// Maybe this whole class can be deleted, or reworked (see TODO in refreshDisplay)

void DisplayManager::init()
{
    DEV_Module_Init();
    Paint_NewImage(_imageData, Config::Display::width, Config::Display::height, 0, WHITE);
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
