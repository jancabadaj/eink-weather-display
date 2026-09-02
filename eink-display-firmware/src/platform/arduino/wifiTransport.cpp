#include "wifiTransport.h"

#include <WiFi.h>

#include "logger.h"
#include "web/requestParser.h"

namespace
{
    WiFiServer server(80);
    constexpr uint64_t requestTimeoutMs = 2000;

    void writeResponse(WiFiClient &client, const Web::Response &response)
    {
        client.println(("HTTP/1.1 " + std::to_string(response.status) +
                        (response.status == 303 ? " See Other" : response.status == 404 ? " Not Found"
                                                                                        : " OK"))
                           .c_str());
        if (!response.location.empty())
        {
            client.println(("Location: " + response.location).c_str());
        }
        if (!response.body.empty())
        {
            client.println(("Content-type: " + response.contentType).c_str());
        }
        client.println("Connection: close");
        client.println();

        if (!response.body.empty())
        {
            client.print(response.body.c_str());
            client.println();
        }
    }
} // namespace

void WifiTransport::begin()
{
    server.begin();
    logger.info("[WifiTransport] HTTP server started");
}

void WifiTransport::poll()
{
    WiFiClient client = server.available();
    if (!client)
    {
        return;
    }

    const uint64_t startedAt = _clock.uptimeMs();
    std::string requestLine;
    bool haveRequestLine = false;
    std::string currentLine;

    while (client.connected() && _clock.uptimeMs() - startedAt <= requestTimeoutMs)
    {
        if (!client.available())
        {
            continue;
        }

        const char c = client.read();
        if (c == '\r')
        {
            continue;
        }

        if (c != '\n')
        {
            currentLine += c;
            continue;
        }

        if (!haveRequestLine)
        {
            requestLine = currentLine;
            haveRequestLine = true;
        }

        // A blank line closes the header block, only then is the request complete
        if (currentLine.empty())
        {
            Web::Request request;
            const Web::Response response = parseRequestLine(requestLine, request)
                                               ? _handler.handle(request)
                                               : Web::Response::notFound();
            writeResponse(client, response);
            break;
        }

        currentLine.clear();
    }

    client.stop();
}
