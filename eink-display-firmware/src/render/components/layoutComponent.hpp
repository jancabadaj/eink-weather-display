
#pragma once

#include <memory>
#include <algorithm>

#include "../drawUtils.h"
#include "../shapes/layout.h"

class LayoutComponent
{
public:
    LayoutComponent(uint8_t *imageData) : _imageData(imageData) {}

    void render()
    {
        DrawUtils::drawShape(_imageData, 0, 0, &Layout, Black);
    }

private:
    uint8_t *_imageData;
};
