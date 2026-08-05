#pragma once

#include <cstdint>
#include <string>

#include "statusSnapshot.h"

namespace Pages
{
    // "2d 3h 4m 5s", trimmed to the largest unit present.
    std::string formatDuration(uint64_t ms);

    std::string renderHome(const StatusSnapshot &status);
} // namespace Pages
