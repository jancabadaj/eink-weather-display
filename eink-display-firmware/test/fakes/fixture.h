#pragma once

#include <fstream>
#include <sstream>
#include <string>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

// Loads a captured API payload from test/fixtures and returns it as a string. Lines starting with "//" are ignored
inline std::string loadFixture(const std::string &name)
{
    std::ifstream in(std::string(PROJECT_ROOT) + "/test/fixtures/" + name);
    std::ostringstream out;
    std::string line;
    while (std::getline(in, line))
    {
        const size_t firstChar = line.find_first_not_of(" \t");
        if (firstChar != std::string::npos && line.compare(firstChar, 2, "//") == 0)
        {
            continue;
        }
        out << line << '\n';
    }
    return out.str();
}
