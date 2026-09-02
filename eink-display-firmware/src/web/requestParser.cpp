#include "requestParser.h"

namespace
{
    Web::Method methodFrom(const std::string &token)
    {
        if (token == "GET")
        {
            return Web::Method::Get;
        }
        if (token == "POST")
        {
            return Web::Method::Post;
        }
        return Web::Method::Unknown;
    }

    void parseQuery(const std::string &query, std::map<std::string, std::string> &out)
    {
        size_t at = 0;
        while (at < query.size())
        {
            size_t end = query.find('&', at);
            if (end == std::string::npos)
            {
                end = query.size();
            }

            const size_t equals = query.find('=', at);
            if (equals != std::string::npos && equals < end)
            {
                out[query.substr(at, equals - at)] = query.substr(equals + 1, end - equals - 1);
            }
            else if (end > at)
            {
                out[query.substr(at, end - at)] = "";
            }

            at = end + 1;
        }
    }
} // namespace

bool parseRequestLine(const std::string &requestLine, Web::Request &out)
{
    const size_t methodEnd = requestLine.find(' ');
    if (methodEnd == std::string::npos)
    {
        return false;
    }

    const size_t targetEnd = requestLine.find(' ', methodEnd + 1);
    if (targetEnd == std::string::npos)
    {
        return false;
    }

    Web::Request parsed;
    parsed.method = methodFrom(requestLine.substr(0, methodEnd));

    const std::string target = requestLine.substr(methodEnd + 1, targetEnd - methodEnd - 1);
    if (target.empty())
    {
        return false;
    }

    const size_t queryStart = target.find('?');
    if (queryStart == std::string::npos)
    {
        parsed.path = target;
    }
    else
    {
        parsed.path = target.substr(0, queryStart);
        parseQuery(target.substr(queryStart + 1), parsed.query);
    }

    out = parsed;
    return true;
}
