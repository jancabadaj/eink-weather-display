#pragma once

#include "../httpClient.h"

// Wrapper around Arduino's HTTPClient. No additional features, just implements the HttpClient interface for the firmware to use.
class ArduinoHttpClient : public HttpClient
{
public:
    HttpResponse get(const std::string &url, const std::string &bearerToken) override;
    HttpResponse postForm(const std::string &url, const std::string &body) override;
};
