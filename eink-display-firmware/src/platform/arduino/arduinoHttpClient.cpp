#include "arduinoHttpClient.h"

#include <HTTPClient.h>

HttpResponse ArduinoHttpClient::get(const std::string &url, const std::string &bearerToken)
{
    HTTPClient http;
    if (!http.begin(url.c_str()))
    {
        return {-1, ""};
    }

    if (!bearerToken.empty())
    {
        http.addHeader("Authorization", ("Bearer " + bearerToken).c_str());
    }

    HttpResponse response;
    response.status = http.GET();
    response.body = http.getString().c_str();
    http.end();
    return response;
}

HttpResponse ArduinoHttpClient::postForm(const std::string &url, const std::string &body)
{
    HTTPClient http;
    if (!http.begin(url.c_str()))
    {
        return {-1, ""};
    }

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    HttpResponse response;
    response.status = http.POST(body.c_str());
    response.body = http.getString().c_str();
    http.end();
    return response;
}
