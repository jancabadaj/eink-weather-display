#pragma once

#include <string>

#include "../logSink.h"

// Sends logs to Google Sheets via AppScript if enabled and WiFi is connected
class SheetsSink : public LogSink
{
public:
    SheetsSink(const char *deploymentId, const char *apiKey);

    bool enabled() const { return _enabled; }

    void write(LogLevel level, const char *message) override;

private:
    std::string _deploymentId;
    std::string _apiKey;
    bool _enabled = false;
};
