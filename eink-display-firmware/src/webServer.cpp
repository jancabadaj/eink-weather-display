#include "webServer.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"
#include "DEV_Config.h"
#include "EPD.h"

// API data - TODO: Move to somewhere else?
String uniqueState = "hello_test_unique"; // TODO: State - according to doc should be arbitrary but unique string

// Timeout handling
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// Web server port number
WiFiServer server(80);

// Variable to store the HTTP request
String header;

void WebServer::init()
{
    server.begin();
    Serial.println("HTTP server started");
}

void WebServer::loop()
{
    WiFiClient client = server.available(); // Listen for incoming clients

    if (client)
    { // If a new client connects,
        previousTime = millis();
        Serial.println("New Client.");
        String currentLine = ""; // make a String to hold incoming data from the client
        while (client.connected() && millis() - previousTime <= timeoutTime)
        { // loop while the client's connected
            if (client.available())
            {                           // if there's bytes to read from the client,
                char c = client.read(); // read a byte, then
                Serial.write(c);        // print it out the serial monitor
                header += c;
                if (c == '\n')
                { // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        handleRequest(client);

                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        // HTML page head
                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; font-size: 30px; margin: 2px; cursor: pointer;}");
                        client.println("</style></head>");

                        // HTML page content
                        client.println("<body><h1>ESP32 Web Server</h1>");
                        client.println("<p>Current time : " + String(millis()) + "</p><br/>");

                        // Display current state
                        auto authData = _auth->getAuthData();
                        client.println("<p>AccessToken : " + authData.accessToken + "</p>");
                        client.println("<p>RefreshToken : " + authData.refreshToken + "</p>");
                        client.println("<p>ExpirationTime : " + String(authData.tokenExpirationTime) + "</p>");

                        // Login button
                        String loginUri = "https://api.netatmo.com/oauth2/authorize?client_id=" + String(config::apiClientId) +
                                          "&redirect_uri=http://" + WiFi.localIP().toString() +
                                          "&scope=read_station&state=" + uniqueState;
                        client.println("<p><a href=\"" + loginUri + "\"><button class=\"button\">Login</button></a></p>");

                        client.println("<p><a href=\"/data/get\"><button class=\"button\">Retrieve data</button></a></p>");

                        client.println("<p><a href=\"/display/print\"><button class=\"button\">Print data</button></a></p>");

                        client.println("<p><a href=\"/display/clear\"><button class=\"button\">Clear display</button></a></p>");

                        // The HTTP response ends with another blank line
                        client.println("</body></html>");
                        client.println();
                        // Break out of the while loop
                        break;
                    }
                    else
                    { // if you got a newline, then clear currentLine
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                }
            }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
    }
}

void WebServer::handleRequest(WiFiClient &client)
{
    if (header.indexOf("GET /display/print") >= 0)
    {
        _weatherCore->drawWeatherData();
        return;
    }
    if (header.indexOf("GET /display/clear") >= 0)
    {
        _weatherCore->clearDisplay();
        return;
    }
    if (header.indexOf("GET /data/get") >= 0)
    {
        Serial.println("Get data");
        _weatherCore->reloadData();
        return;
    }

    // Check if request is redirection from login page
    String search = "GET /?state=" + uniqueState + "&code=";
    int index = header.indexOf(search);
    if (index >= 0) // Obtain token using authorization code
    {
        Serial.println("Authenticate");
        String code = header.substring(index + search.length(), header.indexOf(" ", index + search.length()));
        _auth->login(code);
        return;
    }
}