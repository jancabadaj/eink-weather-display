#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "config.h"

// Web server port number
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// API data
String accessToken = "";
String refreshToken = "";
unsigned long tokenExpirationTime = 0;
String uniqueState = "hello_test_unique"; // TODO: State - according to doc should be arbitrary but unique string

// Timeout handling
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void handleRequest(WiFiClient &client);

void setup()
{
  Serial.begin(115200);

  Serial.print("Connecting to ");
  Serial.println(config::wifiSsid);
  WiFi.begin(config::wifiSsid, config::wifiPassword);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop()
{
  WiFiClient client = server.available(); // Listen for incoming clients

  Serial.print("-");
  delay(500);

  if (client)
  { // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = ""; // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
      if (client.available())
      {                         // if there's bytes to read from the client,
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
            client.println("<p>Current time : " + String(currentTime) + "</p><br/>");

            // Display current state
            client.println("<p>AccessToken : " + accessToken + "</p>");
            client.println("<p>RefreshToken : " + refreshToken + "</p>");
            client.println("<p>ExpirationTime : " + String(tokenExpirationTime) + "</p>");

            // Login button
            String loginUri = "https://api.netatmo.com/oauth2/authorize?client_id=" + String(config::apiClientId) +
                              "&redirect_uri=http://" + WiFi.localIP().toString() +
                              "&scope=read_station&state=" + uniqueState;
            client.println("<p><a href=\"" + loginUri + "\"><button class=\"button\">Login</button></a></p>");

            client.println("<p><a href=\"/data/get\"><button class=\"button\">Retrieve data</button></a></p>");

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
        {                   // if you got anything else but a carriage return character,
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

void handleRequest(WiFiClient &client)
{
  if (header.indexOf("GET /data/get") >= 0)
  {
    Serial.println("Get data");

    HTTPClient http;
    String serverPath = "https://api.netatmo.com/api/getstationsdata";
    http.begin(serverPath.c_str());
    http.addHeader("Authorization", "Bearer " + accessToken);

    // Send HTTP GET request
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);

      // TODO: Parse response
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
    return;
  }

  // Check if request is redirection from login page
  String search = "GET /?state=" + uniqueState + "&code=";
  int index = header.indexOf(search);
  if (index >= 0) // Obtain token using authorization code
  {
    HTTPClient http;
    String serverPath = "https://api.netatmo.com/oauth2/token";
    http.begin(serverPath.c_str());

    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String code = header.substring(index + search.length(), header.indexOf(" ", index + search.length()));

    // Data to send with HTTP POST
    String httpRequestData = String("") +
                             "grant_type=authorization_code" + "&" +
                             "client_id=" + String(config::apiClientId) + "&" +
                             "client_secret=" + String(config::apiClientSecret) + "&" +
                             "code=" + code + "&" +
                             "redirect_uri=http://" + WiFi.localIP().toString() + "&" +
                             "scope=read_station";

    Serial.print("body: ");
    Serial.println(httpRequestData);

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);

      StaticJsonDocument<384> doc;

      // Deserialize the JSON document
      DeserializationError error = deserializeJson(doc, payload);

      // Test if parsing succeeds.
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      // Fetch values.
      //
      // Most of the time, you can rely on the implicit casts.
      // In other case, you can do doc["time"].as<long>();
      const char *access_token = doc["access_token"];
      const char *refresh_token = doc["refresh_token"];
      long expires_in = doc["expires_in"];

      accessToken = String(access_token);
      refreshToken = String(refresh_token);
      tokenExpirationTime = currentTime + expires_in * 1000; // Expiration time is in seconds

      // TODO: before getting station data, check expiration time if still valid (maybe need to store current time when requesting token)
      //       if not valid, auto request new one with refrfesh token
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
    return;
  }
}