#pragma once

#include <Arduino.h>

#include "../../logger.h"
#include "../logSink.h"

class SerialSink : public LogSink
{
public:
    void write(LogLevel level, const char *message) override
    {
        Serial.print("[");
        Serial.print(Logger::levelToString(level));
        Serial.print("] ");
        Serial.println(message);
    }
};
