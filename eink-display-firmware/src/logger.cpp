#include "logger.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Global logger instance
Logger logger;

void Logger::init(const char *deploymentId, const char *apiKey)
{
    // Check if both credentials are provided
    if (deploymentId != nullptr && apiKey != nullptr &&
        strlen(deploymentId) > 0 && strlen(apiKey) > 0)
    {
        _deploymentId = deploymentId;
        _apiKey = apiKey;
        _googleSheetsEnabled = true;
        info("[Logger] Google Sheets logging enabled");
    }
    else
    {
        _googleSheetsEnabled = false;
        info("[Logger] Serial-only logging (no Google Sheets credentials)");
    }
}

void Logger::setLogLevel(LogLevel level)
{
    _minLevel = level;
    info("[Logger] Log level set to %s", levelToString(level));
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
    // Format the message
    vsnprintf(_buffer, LOG_BUFFER_SIZE, format, args);

    // Always print to Serial
    Serial.print("[");
    Serial.print(levelToString(level));
    Serial.print("] ");
    Serial.println(_buffer);

    // Send to Google Sheets if enabled and WiFi is connected
    if (level >= _minLevel && _googleSheetsEnabled && WiFi.status() == WL_CONNECTED)
    {
        sendToGoogleSheets(_buffer, level);
    }
}

void Logger::sendToGoogleSheets(const char *message, LogLevel level)
{
    HTTPClient http;

    // Build Google Apps Script URL
    const std::string url = "https://script.google.com/macros/s/" + _deploymentId + "/exec";

    if (!http.begin(url.c_str()))
    {
        return; // Silently fail
    }

    // Set headers for form data
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // URL encode the log message (basic encoding)
    std::string encodedMessage;
    encodedMessage.reserve(strlen(message) + 8);
    for (const char *c = message; *c; c++)
    {
        switch (*c)
        {
        case ' ':
            encodedMessage += '+';
            break;
        case '\n':
            encodedMessage += "%0A";
            break;
        case '\r':
            encodedMessage += "%0D";
            break;
        default:
            encodedMessage += *c;
        }
    }

    const std::string postData = "key=" + _apiKey +
                                 "&log=" + encodedMessage +
                                 "&level=" + levelToString(level);

    // Send POST request (non-blocking, fire and forget)
    http.POST(postData.c_str());
    http.end();
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
