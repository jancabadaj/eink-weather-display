#pragma once

#include "httpMessage.h"

class RequestHandler
{
public:
    virtual ~RequestHandler() = default;

    virtual Web::Response handle(const Web::Request &request) = 0;
};
