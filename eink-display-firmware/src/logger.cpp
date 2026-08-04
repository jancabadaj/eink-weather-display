#include "logger.h"

#include <cstdio>

// Global logger instance
Logger logger;

void Logger::addSink(LogSink &sink, LogLevel minLevel)
{
    if (_sinkCount < maxSinks)
    {
        _sinks[_sinkCount++] = {&sink, minLevel};
    }
}

void Logger::clearSinks()
{
    _sinkCount = 0;
}

void Logger::debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::DEBUG, format, args);
    va_end(args);
}

void Logger::info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::INFO, format, args);
    va_end(args);
}

void Logger::warning(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::WARNING, format, args);
    va_end(args);
}

void Logger::error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::ERROR, format, args);
    va_end(args);
}

void Logger::critical(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(LogLevel::CRITICAL, format, args);
    va_end(args);
}

void Logger::log(LogLevel level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logInternal(level, format, args);
    va_end(args);
}

void Logger::logInternal(LogLevel level, const char *format, va_list args)
{
    size_t interested = 0;
    for (size_t i = 0; i < _sinkCount; i++)
    {
        if (level >= _sinks[i].minLevel)
        {
            interested++;
        }
    }

    if (interested == 0)
    {
        return; // nothing to format for
    }

    vsnprintf(_buffer, LOG_BUFFER_SIZE, format, args);

    for (size_t i = 0; i < _sinkCount; i++)
    {
        if (level >= _sinks[i].minLevel)
        {
            _sinks[i].sink->write(level, _buffer);
        }
    }
}

const char *Logger::levelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARNING:
        return "WARN";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}
