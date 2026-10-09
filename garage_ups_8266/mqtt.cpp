#include "mqtt.h"

#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <math.h>
#include <stdio.h>

#include "secrets.h"
#include "sensors.h"
#include "outputs.h"
#include "protection.h"

static constexpr char AIO_SERVER[] = "io.adafruit.com";
static constexpr uint16_t AIO_SERVERPORT = 8883;
static constexpr unsigned long MQTT_PUBLISH_INTERVAL_MS = 30000UL;
static constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS = 30000UL;

// Topic buffers live for the entire program lifetime. They are filled before
// constructing the MQTT publisher objects, so no temporary String pointers
// are retained by Adafruit_MQTT_Publish.
static char topicBatteryVoltage[96];
static char topicSourceVoltage[96];
static char topicChargeCurrent[96];
static char topicBatteryTemperature[96];
static char topicExternalTemperature[96];
static char topicSourcePresent[96];
static char topicChargeRelay[96];
static char topicLoad0[96];
static char topicLoad1[96];
static char topicProtection[96];

static BearSSL::WiFiClientSecure secureClient;
static Adafruit_MQTT_Client* mqtt = nullptr;
static Adafruit_MQTT_Publish* feedBatteryVoltage = nullptr;
static Adafruit_MQTT_Publish* feedSourceVoltage = nullptr;
static Adafruit_MQTT_Publish* feedChargeCurrent = nullptr;
static Adafruit_MQTT_Publish* feedBatteryTemperature = nullptr;
static Adafruit_MQTT_Publish* feedExternalTemperature = nullptr;
static Adafruit_MQTT_Publish* feedSourcePresent = nullptr;
static Adafruit_MQTT_Publish* feedChargeRelay = nullptr;
static Adafruit_MQTT_Publish* feedLoad0 = nullptr;
static Adafruit_MQTT_Publish* feedLoad1 = nullptr;
static Adafruit_MQTT_Publish* feedProtection = nullptr;

static bool mqttReady = false;
static bool mqttInitialized = false;
static unsigned long lastReconnectAttempt = 0;
static unsigned long lastPublish = 0;

static bool validValue(float value) {
    return isfinite(value);
}

static bool makeTopic(char* destination, size_t capacity, const char* feedName) {
    const int written = snprintf(destination, capacity, "%s/feeds/%s", AIO_USERNAME.c_str(), feedName);
    return written > 0 && static_cast<size_t>(written) < capacity;
}

static bool connectMqtt() {
    if (WiFi.status() != WL_CONNECTED || mqtt == nullptr) return false;

    Serial.println(F("MQTT: connecting to Adafruit IO..."));
    const int8_t result = mqtt->connect();
    if (result == 0) {
        mqttReady = true;
        Serial.println(F("MQTT: connected."));
        return true;
    }

    Serial.print(F("MQTT: connection failed, code "));
    Serial.println(result);
    mqtt->disconnect();
    mqttReady = false;
    return false;
}

static uint8_t protectionCode() {
    if (protectionIsOverheatLatched()) return 1;
    if (!protectionTemperatureSensorsOk()) return 2;
    if (protectionIsLowBatteryCutoff()) return 3;
    if (!protectionBatteryMeasurementOk()) return 4;
    return 0;
}

void mqttBegin() {
    if (mqttInitialized) return;

    // NOTE: setInsecure encrypts traffic but does not validate the server
    // certificate. Configure CA/certificate validation before relying on this
    // connection for production use.
    secureClient.setInsecure();

    const bool topicsOk =
        makeTopic(topicBatteryVoltage, sizeof(topicBatteryVoltage), "battery-voltage") &&
        makeTopic(topicSourceVoltage, sizeof(topicSourceVoltage), "source-voltage") &&
        makeTopic(topicChargeCurrent, sizeof(topicChargeCurrent), "charge-current") &&
        makeTopic(topicBatteryTemperature, sizeof(topicBatteryTemperature), "battery-temperature") &&
        makeTopic(topicExternalTemperature, sizeof(topicExternalTemperature), "external-temperature") &&
        makeTopic(topicSourcePresent, sizeof(topicSourcePresent), "source-present") &&
        makeTopic(topicChargeRelay, sizeof(topicChargeRelay), "charge-relay") &&
        makeTopic(topicLoad0, sizeof(topicLoad0), "load-0") &&
        makeTopic(topicLoad1, sizeof(topicLoad1), "load-1") &&
        makeTopic(topicProtection, sizeof(topicProtection), "protection");

    if (!topicsOk) {
        Serial.println(F("MQTT: failed to build feed names; MQTT disabled."));
        return;
    }

    // Create these only after AIO_USERNAME is initialized and topic buffers
    // contain stable strings. The library keeps pointers to these names.
    mqtt = new Adafruit_MQTT_Client(&secureClient, AIO_SERVER, AIO_SERVERPORT,
                                    AIO_USERNAME.c_str(), AIO_KEY);
    if (mqtt == nullptr) {
        Serial.println(F("MQTT: failed to allocate client; MQTT disabled."));
        return;
    }

    feedBatteryVoltage = new Adafruit_MQTT_Publish(mqtt, topicBatteryVoltage);
    feedSourceVoltage = new Adafruit_MQTT_Publish(mqtt, topicSourceVoltage);
    feedChargeCurrent = new Adafruit_MQTT_Publish(mqtt, topicChargeCurrent);
    feedBatteryTemperature = new Adafruit_MQTT_Publish(mqtt, topicBatteryTemperature);
    feedExternalTemperature = new Adafruit_MQTT_Publish(mqtt, topicExternalTemperature);
    feedSourcePresent = new Adafruit_MQTT_Publish(mqtt, topicSourcePresent);
    feedChargeRelay = new Adafruit_MQTT_Publish(mqtt, topicChargeRelay);
    feedLoad0 = new Adafruit_MQTT_Publish(mqtt, topicLoad0);
    feedLoad1 = new Adafruit_MQTT_Publish(mqtt, topicLoad1);
    feedProtection = new Adafruit_MQTT_Publish(mqtt, topicProtection);

    if (!feedBatteryVoltage || !feedSourceVoltage || !feedChargeCurrent ||
        !feedBatteryTemperature || !feedExternalTemperature || !feedSourcePresent ||
        !feedChargeRelay || !feedLoad0 || !feedLoad1 || !feedProtection) {
        Serial.println(F("MQTT: failed to allocate one or more feeds; MQTT disabled."));
        return;
    }

    mqttInitialized = true;
    mqttReady = false;
    lastReconnectAttempt = 0;
    lastPublish = millis();
    Serial.println(F("MQTT telemetry initialized (TLS certificate validation disabled)."));
}

void mqttUpdate() {
    if (!mqttInitialized || mqtt == nullptr) return;
    const unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        mqttReady = false;
        return;
    }

    if (!mqtt->connected()) {
        mqttReady = false;
        if (lastReconnectAttempt == 0 || now - lastReconnectAttempt >= MQTT_RECONNECT_INTERVAL_MS) {
            lastReconnectAttempt = now;
            connectMqtt();
        }
        return;
    }

    mqttReady = true;
    mqtt->processPackets(1);

    if (now - lastPublish < MQTT_PUBLISH_INTERVAL_MS) return;
    lastPublish = now;

    // Ten feed writes every 30 seconds = about 20 writes/minute.
    bool ok = true;
    const float battery = fabs(sensorsBatteryVoltage());
    const float source = fabs(sensorsChargerVoltage());
    const float current = fabs(sensorsCurrentAmps());
    const float batteryTemp = sensorsNtcTemperature();
    const float externalTemp = sensorsDs18b20Temperature();

    if (validValue(battery)) ok &= feedBatteryVoltage->publish(battery);
    if (validValue(source)) ok &= feedSourceVoltage->publish(source);
    if (validValue(current)) ok &= feedChargeCurrent->publish(current < 0.0f ? 0.0f : current);
    if (validValue(batteryTemp)) ok &= feedBatteryTemperature->publish(batteryTemp);
    if (validValue(externalTemp) && externalTemp > -100.0f) ok &= feedExternalTemperature->publish(externalTemp);

    ok &= feedSourcePresent->publish(protectionIsChargerPresent() ? 1 : 0);
    ok &= feedChargeRelay->publish(outputsGetSafety() ? 1 : 0);
    ok &= feedLoad0->publish(outputsGetLoad0() ? 1 : 0);
    ok &= feedLoad1->publish(outputsGetLoad1() ? 1 : 0);
    ok &= feedProtection->publish(protectionCode());

    Serial.println(ok ? F("MQTT: telemetry published.") : F("MQTT: one or more publishes failed."));
}
