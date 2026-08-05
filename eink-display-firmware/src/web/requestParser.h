#pragma once

#include <string>

#include "httpMessage.h"

// Reads the request line only - for example "GET /path?a=1&b=2 HTTP/1.1"
// The rest of the request including headers or cookies is deliberately not checked
bool parseRequestLine(const std::string &requestLine, Web::Request &out);
