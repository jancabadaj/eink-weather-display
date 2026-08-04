#pragma once

#include <string>

class Network
{
public:
    virtual ~Network() = default;

    virtual bool isConnected() const = 0;
    virtual std::string localAddress() const = 0;
};
