#include <doctest.h>

#include "web/requestParser.h"

TEST_CASE("RequestParser: readsMethodAndPath")
{
    Web::Request request;
    REQUIRE(parseRequestLine("GET /config/set HTTP/1.1", request));

    CHECK(request.method == Web::Method::Get);
    CHECK(request.path == "/config/set");
    CHECK(request.query.empty());
}

TEST_CASE("RequestParser: readsQueryParameters")
{
    Web::Request request;
    REQUIRE(parseRequestLine("GET /config/set?night_start=22&night_end=6 HTTP/1.1", request));

    CHECK(request.path == "/config/set");
    CHECK(request.param("night_start") == "22");
    CHECK(request.param("night_end") == "6");
}

TEST_CASE("RequestParser: ignoresHeadersEntirely")
{
    // A Referer carrying an earlier OAuth redirect must not look like a parameter
    // of this request.
    const std::string requestLine = "GET / HTTP/1.1";
    Web::Request request;
    REQUIRE(parseRequestLine(requestLine, request));

    CHECK(request.param("state").empty());
    CHECK(request.param("code").empty());
}

TEST_CASE("RequestParser: parameterValueStopsAtItsOwnDelimiter")
{
    Web::Request request;
    REQUIRE(parseRequestLine("GET /?code=abc&state=xyz HTTP/1.1", request));

    // Neither value may run on into the next parameter or the HTTP version.
    CHECK(request.param("code") == "abc");
    CHECK(request.param("state") == "xyz");
}

TEST_CASE("RequestParser: doesNotMatchParameterNamesBySuffix")
{
    Web::Request request;
    REQUIRE(parseRequestLine("GET /config/set?night_start=22&night_end=6 HTTP/1.1", request));

    // "start" and "end" are not parameters here, even though they are tails of
    // ones that are.
    CHECK_FALSE(request.has("start"));
    CHECK_FALSE(request.has("end"));
}

TEST_CASE("RequestParser: handlesValuelessAndEmptyParameters")
{
    Web::Request request;
    REQUIRE(parseRequestLine("GET /?flag&empty= HTTP/1.1", request));

    CHECK(request.has("flag"));
    CHECK(request.param("flag").empty());
    CHECK(request.has("empty"));
    CHECK(request.param("empty").empty());
}

TEST_CASE("RequestParser: rejectsMalformedRequestLines")
{
    Web::Request request;
    CHECK_FALSE(parseRequestLine("", request));
    CHECK_FALSE(parseRequestLine("GET", request));
    CHECK_FALSE(parseRequestLine("GET /nohttpversion", request));
}

TEST_CASE("RequestParser: reportsUnknownMethods")
{
    Web::Request request;
    REQUIRE(parseRequestLine("DELETE / HTTP/1.1", request));
    CHECK(request.method == Web::Method::Unknown);
}
