#pragma once

#include <string>
#include <vector>

#include "platform/logSink.h"

// Records log messages in memory for later inspection in tests
class CapturingLogSink : public LogSink
{
public:
    struct Line
    {
        LogLevel level;
        std::string message;
    };

    void write(LogLevel level, const char *message) override
    {
        lines.push_back({level, message});
    }

    bool contains(const std::string &fragment) const
    {
        for (const auto &l : lines)
        {
            if (l.message.find(fragment) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<Line> lines;
};
