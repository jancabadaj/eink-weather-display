#pragma once

#include "capturingLogSink.h"
#include "logger.h"

// Attaches a capturing sink to the global logger for the lifetime of one test case
// The logger is a global, so a sink attached without scoping would stay attached for every test that follows
class ScopedLogCapture
{
public:
    ScopedLogCapture(LogLevel minLevel = LogLevel::DEBUG)
    {
        logger.clearSinks();
        logger.addSink(sink, minLevel);
    }

    ~ScopedLogCapture() { logger.clearSinks(); }

    ScopedLogCapture(const ScopedLogCapture &) = delete;
    ScopedLogCapture &operator=(const ScopedLogCapture &) = delete;

    CapturingLogSink sink;
};
