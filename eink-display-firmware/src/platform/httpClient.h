#pragma once

#include <string>

struct HttpResponse
{
    int status = 0; // HTTP status code, or negative on transport failure
    std::string body;

    bool ok() const { return status == 200; }
};

class HttpClient
{
public:
    virtual ~HttpClient() = default;

    virtual HttpResponse get(const std::string &url, const std::string &bearerToken) = 0;
    virtual HttpResponse postForm(const std::string &url, const std::string &body) = 0;
};
