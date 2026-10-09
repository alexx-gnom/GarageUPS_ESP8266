#include "web.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <math.h>

#include "secrets.h"
#include "sensors.h"
#include "outputs.h"
#include "protection.h"
#include "web_page.h"

static ESP8266WebServer server(80);
static bool serverStarted = false;

static String jsonFloat(float value, uint8_t decimals) {
    if (!isfinite(value)) return "null";
    return String(value, decimals);
}

static void handleRoot() {
    server.send_P(200, "text/html", MAIN_PAGE);
}

static void handleStatus() {
    String json;
    json.reserve(320);
    json = "{";
    json += "\"charger\":" + jsonFloat(sensorsChargerVoltage(), 2);
    json += ",\"battery\":" + jsonFloat(sensorsBatteryVoltage(), 2);
    json += ",\"current\":" + jsonFloat(sensorsCurrentAmps(), 2);
    json += ",\"ntc\":" + jsonFloat(sensorsNtcTemperature(), 1);
    json += ",\"ds18\":" + jsonFloat(sensorsDs18b20Temperature(), 2);

    json += ",\"chargeRelay\":" + String(outputsGetSafety() ? "true" : "false");
    json += ",\"chargeRequest\":" + String(protectionGetChargerRequest() ? "true" : "false");

    json += ",\"load0\":" + String(outputsGetLoad0() ? "true" : "false");
    json += ",\"load1\":" + String(outputsGetLoad1() ? "true" : "false");
    json += ",\"load0Desired\":" + String(protectionGetLoad0Desired() ? "true" : "false");
    json += ",\"load1Desired\":" + String(protectionGetLoad1Desired() ? "true" : "false");
    json += ",\"load0Restarting\":" + String(protectionIsLoad0Restarting() ? "true" : "false");
    json += ",\"load1Restarting\":" + String(protectionIsLoad1Restarting() ? "true" : "false");

    json += ",\"overheat\":" + String(protectionIsOverheatLatched() ? "true" : "false");
    json += ",\"temperatureOk\":" + String(protectionTemperatureSensorsOk() ? "true" : "false");
    json += ",\"lowBattery\":" + String(protectionIsLowBatteryCutoff() ? "true" : "false");
    json += ",\"loadsAllowed\":" + String(protectionAllowsLoads() ? "true" : "false");
    json += ",\"chargerPresent\":" + String(protectionIsChargerPresent() ? "true" : "false");
    json += ",\"batteryOk\":" + String(protectionBatteryMeasurementOk() ? "true" : "false");
    json += "}";

    server.send(200, "application/json", json);
}

static void handleChargerToggle() {
    const bool requested = !protectionGetChargerRequest();
    protectionSetChargerRequest(requested);
    server.send(200, "application/json", String("{\"ok\":true,\"requested\":") + (requested ? "true}" : "false}"));
}

static void handleLoad0Toggle() {
    if (!protectionToggleLoadDesired(0)) {
        server.send(500, "application/json", "{\"ok\":false,\"error\":\"storage\"}");
        return;
    }
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleLoad1Toggle() {
    if (!protectionToggleLoadDesired(1)) {
        server.send(500, "application/json", "{\"ok\":false,\"error\":\"storage\"}");
        return;
    }
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handleLoad0Restart() {
    if (!protectionRestartLoad(0)) {
        server.send(400, "application/json", "{\"ok\":false}");
        return;
    }
    server.send(200, "application/json", "{\"ok\":true,\"delaySeconds\":7}");
}

static void handleLoad1Restart() {
    if (!protectionRestartLoad(1)) {
        server.send(400, "application/json", "{\"ok\":false}");
        return;
    }
    server.send(200, "application/json", "{\"ok\":true,\"delaySeconds\":7}");
}

bool webBegin() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Connecting to Wi-Fi");
    const unsigned long timeoutMs = 20000;
    const unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < timeoutMs) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi connection failed.");
        return false;
    }

    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/charger/toggle", HTTP_POST, handleChargerToggle);
    server.on("/api/load0/toggle", HTTP_POST, handleLoad0Toggle);
    server.on("/api/load1/toggle", HTTP_POST, handleLoad1Toggle);
    server.on("/api/load0/restart", HTTP_POST, handleLoad0Restart);
    server.on("/api/load1/restart", HTTP_POST, handleLoad1Restart);

    server.begin();
    serverStarted = true;
    Serial.println("HTTP server started.");
    return true;
}

void webHandleClient() {
    if (serverStarted) server.handleClient();
}
