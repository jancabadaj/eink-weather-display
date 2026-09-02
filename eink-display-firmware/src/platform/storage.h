#pragma once

#include <string>

// Namespaced key/value persistence (NVS on device, in-memory in tests).
class Storage
{
public:
    virtual ~Storage() = default;

    virtual std::string getString(const char *key, const char *fallback = "") = 0;
    virtual void putString(const char *key, const std::string &value) = 0;

    virtual int getInt(const char *key, int fallback) = 0;
    virtual void putInt(const char *key, int value) = 0;

    virtual void clear() = 0;
};
