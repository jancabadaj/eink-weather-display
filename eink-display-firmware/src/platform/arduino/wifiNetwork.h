#pragma once

#include "../network.h"

class WifiNetwork : public Network
{
public:
    bool isConnected() const override;
    std::string localAddress() const override;
};
