#include "web.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "secrets.h"
#include "sensors.h"
#include "outputs.h"
#include "web_page.h"

static ESP8266WebServer server(80);
static bool serverStarted = false;

static void handleRoot()
{
    server.send_P(200, "text/html", MAIN_PAGE);
}

// -----------------------------------------------------
// JSON status API
// -----------------------------------------------------

static void handleStatus()
{
    String json = "{";

    json += "\"charger\":" +
            String(sensorsChargerVoltage(), 2);

    json += ",\"battery\":" +
            String(sensorsBatteryVoltage(), 2);

    json += ",\"current\":" +
            String(sensorsCurrentAmps(), 2);

    json += ",\"ntc\":" +
            String(sensorsNtcTemperature(), 1);

    json += ",\"ds18\":" +
            String(sensorsDs18b20Temperature(), 2);

    json += ",\"safety\":" +
            String(outputsGetSafety() ? "true" : "false");

    json += ",\"load0\":" +
            String(outputsGetLoad0() ? "true" : "false");

    json += ",\"load1\":" +
            String(outputsGetLoad1() ? "true" : "false");

    json += "}";

    server.send(200, "application/json", json);
}

static void handleSafetyToggle()
{
    outputsSetSafety(!outputsGetSafety());
    server.send(200, "text/plain", "OK");
}

static void handleLoad0Toggle()
{
    outputsSetLoad0(!outputsGetLoad0());
    server.send(200, "text/plain", "OK");
}

static void handleLoad1Toggle()
{
    outputsSetLoad1(!outputsGetLoad1());
    server.send(200, "text/plain", "OK");
}

// -----------------------------------------------------
// Start Wi-Fi and server
// -----------------------------------------------------

bool webBegin()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Connecting to Wi-Fi");

    const unsigned long timeoutMs = 20000;
    const unsigned long startedAt = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - startedAt < timeoutMs)
    {
        delay(250);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Wi-Fi connection failed.");
        return false;
    }

    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);

    server.on("/api/safety/toggle",
              HTTP_POST, handleSafetyToggle);

    server.on("/api/load0/toggle",
              HTTP_POST, handleLoad0Toggle);

    server.on("/api/load1/toggle",
              HTTP_POST, handleLoad1Toggle);

    server.begin();
    serverStarted = true;

    Serial.println("HTTP server started.");

    return true;
}

void webHandleClient()
{
    if (serverStarted)
        server.handleClient();
}
