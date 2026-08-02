#pragma once

#include "../config.h"

class LogSink
{
public:
    virtual ~LogSink() = default;

    virtual void write(LogLevel level, const char *message) = 0;
};
