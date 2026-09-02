#include "epdPanel.h"

#include "DEV_Config.h"
#include "utility/EPD_7in5_V2.h"

// TODO: Paint_NewImage should be removed and DEV_Module_Init can be moved to main.cpp
// Maybe this whole class can be deleted, or reworked (see TODO in refreshDisplay)
void EpdPanel::init()
{
    DEV_Module_Init();
}

// TODO: Figure out better way to init + sleep (smart pointers? init when creating and sleep when leaving scope)
void EpdPanel::present(const uint8_t *frame)
{
    EPD_7IN5_V2_Init();
    // The vendor driver takes a non-const pointer but only reads the buffer.
    EPD_7IN5_V2_Display(const_cast<UBYTE *>(frame));
    EPD_7IN5_V2_Sleep();
}

void EpdPanel::clear()
{
    EPD_7IN5_V2_Init();
    EPD_7IN5_V2_Clear();
    EPD_7IN5_V2_Sleep();
}
