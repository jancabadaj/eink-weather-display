#pragma once

#include <map>
#include <string>

#include "platform/storage.h"

// In-memory storage for tests
class MemStorage : public Storage
{
public:
    std::string getString(const char *key, const char *fallback = "") override
    {
        const auto it = _strings.find(key);
        return it == _strings.end() ? fallback : it->second;
    }

    void putString(const char *key, const std::string &value) override
    {
        _strings[key] = value;
        writes++;
    }

    int getInt(const char *key, int fallback) override
    {
        const auto it = _ints.find(key);
        return it == _ints.end() ? fallback : it->second;
    }

    void putInt(const char *key, int value) override
    {
        _ints[key] = value;
        writes++;
    }

    void clear() override
    {
        _strings.clear();
        _ints.clear();
        clears++;
    }

    bool hasInt(const char *key) const { return _ints.count(key) > 0; }

    int writes = 0;
    int clears = 0;

private:
    std::map<std::string, std::string> _strings;
    std::map<std::string, int> _ints;
};
