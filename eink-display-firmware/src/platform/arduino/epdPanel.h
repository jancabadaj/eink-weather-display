#pragma once

#include <cstdint>

#include "../displayPanel.h"

// Waveshare 7.5" V2 e-ink panel
class EpdPanel : public DisplayPanel
{
public:
    void init();

    void present(const uint8_t *frame) override;
    void clear() override;
};
