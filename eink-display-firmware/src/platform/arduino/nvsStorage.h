#pragma once

#include <Preferences.h>

#include "../storage.h"

// NVS-backed key/value store. Each instance is namespaced, so different parts of the firmware can use the same key names without colliding
class NvsStorage : public Storage
{
public:
    explicit NvsStorage(const char *ns) : _ns(ns) {}

    std::string getString(const char *key, const char *fallback = "") override;
    void putString(const char *key, const std::string &value) override;

    int getInt(const char *key, int fallback) override;
    void putInt(const char *key, int value) override;

    void clear() override;

private:
    const char *_ns;
};
