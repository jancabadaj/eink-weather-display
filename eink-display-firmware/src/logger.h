#pragma once

#include <cstdarg>
#include <cstddef>

#include "config.h"
#include "platform/logSink.h"

class Logger
{
public:
    // There is a single global logger instance which delegates to each attached sink
    void addSink(LogSink &sink, LogLevel minLevel);

    // Logging methods - use printf-style formatting
    void debug(const char *format, ...);
    void info(const char *format, ...);
    void warning(const char *format, ...);
    void error(const char *format, ...);
    void critical(const char *format, ...);
    void log(LogLevel level, const char *format, ...);

    static const char *levelToString(LogLevel level);

private:
    void logInternal(LogLevel level, const char *format, va_list args);

    static constexpr size_t maxSinks = 4;
    struct Attached
    {
        LogSink *sink = nullptr;
        LogLevel minLevel = LogLevel::DEBUG;
    };

    Attached _sinks[maxSinks];
    size_t _sinkCount = 0;

    static constexpr size_t LOG_BUFFER_SIZE = 512;
    char _buffer[LOG_BUFFER_SIZE]{};
};

// Global logger instance
extern Logger logger;
