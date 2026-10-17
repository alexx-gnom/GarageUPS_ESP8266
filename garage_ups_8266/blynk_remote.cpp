
#include "blynk_remote.h"

#include <ESP8266WiFi.h>
#include "secrets.h"
#include <BlynkSimpleEsp8266.h>
#include <math.h>

#include "sensors.h"
#include "outputs.h"
#include "protection.h"

BLYNK_WRITE(V10) {
    if (param.asInt() == 1) {
        const bool accepted = protectionRestartLoad(0);
        Serial.println(accepted
            ? F("Blynk: restart Load 0 requested.")
            : F("Blynk: restart Load 0 rejected."));
    }
    Blynk.virtualWrite(V10, 0); // Reset push button in the app
    blinkSendData();
}

BLYNK_WRITE(V11) {
    if (param.asInt() == 1) {
        const bool accepted = protectionRestartLoad(1);
        Serial.println(accepted
            ? F("Blynk: restart Load 1 requested.")
            : F("Blynk: restart Load 1 rejected."));
    }
    Blynk.virtualWrite(V11, 0); // Reset push button in the app
    blinkSendData();
}

BLYNK_WRITE(V12) {
    protectionSetChargerRequest(param.asInt() != 0);
    Serial.println(F("Blynk: charge request updated."));
    blinkSendData();
}

BLYNK_WRITE(V14) {
     if (param.asInt() == 1) {
        Serial.println(F("Blynk: manual data refresh requested."));
        blinkSendData();
    }
}

static constexpr unsigned long BLYNK_RECONNECT_INTERVAL_MS = 10000UL;
static constexpr unsigned long BLYNK_PUBLISH_INTERVAL_MS = 600000UL;

static unsigned long lastReconnectAttempt = 0;
static unsigned long lastPublish = 0;
static bool blynkInitialized = false;

static uint8_t protectionCode() {
    if (protectionIsOverheatLatched()) return 1;
    if (!protectionTemperatureSensorsOk()) return 2;
    if (protectionIsLowBatteryCutoff()) return 3;
    if (!protectionBatteryMeasurementOk()) return 4;
    return 0;
}

static const char* protectionStatusText() {
    switch (protectionCode()) {
        case 0: return "OK";
        case 1: return "OVERHEAT";
        case 2: return "TEMP SENSOR ERROR";
        case 3: return "LOW BATTERY";
        case 4: return "BATTERY VOLTAGE ERROR";
        default: return "UNKNOWN ERROR";
    }
}

void blynkRemoteBegin() {
    if (blynkInitialized) return;

    Blynk.config(BLYNK_AUTH_TOKEN);
    blynkInitialized = true;
    lastReconnectAttempt = 0;
    lastPublish = millis();

    Serial.println(F("Blynk: initialized."));
}

void blynkRemoteUpdate() {
    if (!blynkInitialized) return;

    const unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) return;

    if (!Blynk.connected()) {
        if (lastReconnectAttempt == 0 ||
            now - lastReconnectAttempt >= BLYNK_RECONNECT_INTERVAL_MS) {
            lastReconnectAttempt = now;
            Serial.println(F("Blynk: connecting..."));

            if (Blynk.connect(100)) {
                Serial.println(F("Blynk: connected."));
                // Publish a fresh snapshot after reconnecting.
                blinkSendData();
            }
        }
        return;
    }

    // Keep the Blynk connection serviced on every main-loop iteration.
    Blynk.run();

    if (millis() - lastPublish >= BLYNK_PUBLISH_INTERVAL_MS) {
        blinkSendData();
    }
}

void blinkSendData()
{
    if (WiFi.status() != WL_CONNECTED || !Blynk.connected()) return;

    // This function only publishes data; Blynk.run() is serviced separately.
    lastPublish = millis();

    const float battery = fabs(sensorsBatteryVoltage());
    const float source = fabs(sensorsChargerVoltage());
    const float current = sensorsCurrentAmps();
    const float batteryTemp = sensorsNtcTemperature();
    const float externalTemp = sensorsDs18b20Temperature();

    if (isfinite(battery)) Blynk.virtualWrite(V0, battery);
    if (isfinite(source)) Blynk.virtualWrite(V1, source);
    if (isfinite(current)) Blynk.virtualWrite(V2, current < 0.0f ? 0.0f : current);
    if (isfinite(batteryTemp)) Blynk.virtualWrite(V3, batteryTemp);
    if (isfinite(externalTemp) && externalTemp > -100.0f) {
        Blynk.virtualWrite(V4, externalTemp);
    }

    Blynk.virtualWrite(V5, protectionIsChargerPresent() ? 1 : 0);
    Blynk.virtualWrite(V6, outputsGetSafety() ? 1 : 0);
    Blynk.virtualWrite(V7, outputsGetLoad0() ? 1 : 0);
    Blynk.virtualWrite(V8, outputsGetLoad1() ? 1 : 0);
    Blynk.virtualWrite(V9, protectionCode());
    Blynk.virtualWrite(V12, protectionGetChargerRequest() ? 1 : 0);
    Blynk.virtualWrite(V13, protectionStatusText());

    Serial.println(F("Blynk: telemetry published."));
}
