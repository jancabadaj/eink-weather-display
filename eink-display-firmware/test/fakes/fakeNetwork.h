#pragma once

#include <string>

#include "platform/network.h"

class FakeNetwork : public Network
{
public:
    bool connected = true;
    std::string address = "192.168.1.50";

    bool isConnected() const override { return connected; }
    std::string localAddress() const override { return address; }
};
