#pragma once

#include <Arduino.h>
#include "config.h"

class Logger
{
public:
    Logger()
        : _minLevel(LogLevel::DEBUG), _googleSheetsEnabled(false) {}

    // Initialize with Google Sheets credentials (empty strings = Serial only)
    void init(const char *deploymentId, const char *apiKey);

    // Set minimum log level (default: DEBUG)
    void setLogLevel(LogLevel level);

    // Logging methods - use printf-style formatting
    void debug(const char *format, ...);
    void info(const char *format, ...);
    void warning(const char *format, ...);
    void error(const char *format, ...);
    void critical(const char *format, ...);

    // Generic log with specified level
    void log(LogLevel level, const char *format, ...);

private:
    void logInternal(LogLevel level, const char *format, va_list args);
    void sendToGoogleSheets(const char *message, LogLevel level);
    const char *levelToString(LogLevel level);

    LogLevel _minLevel;

    // Google Sheets configuration
    String _deploymentId;
    String _apiKey;
    bool _googleSheetsEnabled;

    // Buffer for formatting messages
    static constexpr size_t LOG_BUFFER_SIZE = 512;
    char _buffer[LOG_BUFFER_SIZE]{};
};

// Global logger instance
extern Logger logger;
