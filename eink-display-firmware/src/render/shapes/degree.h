#pragma once

#include "../shape.h"

// DegreeIcon bitmap (19x19)
const uint8_t DegreeIcon_Bitmap[] = {
    0x07, 0xFC, 0x00,  //     #########          
    0x0F, 0xFE, 0x00,  //    ###########         
    0x3F, 0xFF, 0x80,  //  ###############       
    0x3F, 0xFF, 0x80,  //  ###############       
    0x7F, 0xFF, 0xC0,  // #################      
    0xFE, 0x0F, 0xE0,  //#######     #######     
    0xFC, 0x07, 0xE0,  //######       ######     
    0xF8, 0x03, 0xE0,  //#####         #####     
    0xF8, 0x03, 0xE0,  //#####         #####     
    0xF8, 0x03, 0xE0,  //#####         #####     
    0xF8, 0x03, 0xE0,  //#####         #####     
    0xF8, 0x03, 0xE0,  //#####         #####     
    0xFC, 0x07, 0xE0,  //######       ######     
    0xFE, 0x0F, 0xE0,  //#######     #######     
    0x7F, 0xFF, 0xC0,  // #################      
    0x3F, 0xFF, 0x80,  //  ###############       
    0x3F, 0xFF, 0x80,  //  ###############       
    0x0F, 0xFE, 0x00,  //    ###########         
    0x07, 0xFC, 0x00,  //     #########          
};

Shape DegreeIcon = {
    .bitmap = DegreeIcon_Bitmap,
    .width = 19,
    .height = 19
};
