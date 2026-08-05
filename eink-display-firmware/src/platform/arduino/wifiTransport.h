#pragma once

#include "platform/clock.h"
#include "web/requestHandler.h"

// Handles HTTP server over WiFi, parsing requests and writing responses
class WifiTransport
{
public:
    WifiTransport(Clock &clock, RequestHandler &handler) : _clock(clock), _handler(handler) {}

    void begin();
    void poll();

private:
    Clock &_clock;
    RequestHandler &_handler;
};
