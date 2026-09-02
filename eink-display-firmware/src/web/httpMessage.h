#pragma once

#include <map>
#include <string>

namespace Web
{
    enum class Method
    {
        Unknown,
        Get,
        Post
    };

    struct Request
    {
        Method method = Method::Unknown;
        std::string path;
        std::map<std::string, std::string> query;

        bool has(const std::string &key) const { return query.count(key) != 0; }

        std::string param(const std::string &key) const
        {
            const auto found = query.find(key);
            return found == query.end() ? std::string() : found->second;
        }
    };

    struct Response
    {
        int status = 200;
        std::string contentType = "text/html";
        std::string body;
        std::string location; // set for redirects

        static Response html(std::string body)
        {
            Response response;
            response.body = std::move(body);
            return response;
        }

        static Response seeOther(std::string location)
        {
            Response response;
            response.status = 303;
            response.location = std::move(location);
            return response;
        }

        static Response notFound()
        {
            Response response;
            response.status = 404;
            response.contentType = "text/plain";
            response.body = "Not found";
            return response;
        }
    };
} // namespace Web
