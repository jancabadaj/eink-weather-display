#include "wifiNetwork.h"

#include <WiFi.h>

bool WifiNetwork::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

std::string WifiNetwork::localAddress() const
{
    return WiFi.localIP().toString().c_str();
}
