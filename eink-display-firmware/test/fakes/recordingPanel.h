#pragma once

#include <cstdint>
#include <vector>

#include "platform/displayPanel.h"
#include "render/frameBuffer.h"

// Keeps a copy of whatever was last shown, and counts how often
class RecordingPanel : public DisplayPanel
{
public:
    int presentCount = 0;
    int clearCount = 0;
    std::vector<uint8_t> lastFrame;

    void present(const uint8_t *frame) override
    {
        presentCount++;
        lastFrame.assign(frame, frame + FrameBuffer::byteCount);
    }

    void clear() override { clearCount++; }
};
