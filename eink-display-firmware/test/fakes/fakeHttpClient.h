#pragma once

#include <deque>
#include <string>
#include <vector>

#include "platform/httpClient.h"

// Serves queued responses and records what was asked for
class FakeHttpClient : public HttpClient
{
public:
    struct Call
    {
        std::string url;
        std::string bearerToken;
        std::string body;
        bool isPost = false;
    };

    std::deque<HttpResponse> queued;
    std::vector<Call> calls;

    void queueOk(std::string body) { queued.push_back({200, std::move(body)}); }
    void queueStatus(int status) { queued.push_back({status, ""}); }

    HttpResponse get(const std::string &url, const std::string &bearerToken) override
    {
        calls.push_back({url, bearerToken, "", false});
        return take();
    }

    HttpResponse postForm(const std::string &url, const std::string &body) override
    {
        calls.push_back({url, "", body, true});
        return take();
    }

private:
    HttpResponse take()
    {
        if (queued.empty())
        {
            return {-1, ""}; // nothing queued reads as a transport failure
        }
        const HttpResponse next = queued.front();
        queued.pop_front();
        return next;
    }
};
