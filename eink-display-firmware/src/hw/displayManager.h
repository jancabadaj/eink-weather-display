#pragma once

#include <cstdint>

class DisplayManager
{
public:
    DisplayManager(uint8_t *imageData) : _imageData(imageData) {}

    void init();
    void refreshDisplay();
    void clearDisplay();

private:
    uint8_t *_imageData;
};