#include "config.h"
#include "sensors.h"
#include "outputs.h"
#include "debug.h"
#include "web.h"
#include "protection.h"
#include "mqtt.h"

void setup() {
    debugBegin();
    debugPrintBanner();

    Serial.printf("Flash chip: %u bytes\n", ESP.getFlashChipRealSize());
    Serial.printf("Flash configured: %u bytes\n", ESP.getFlashChipSize());
    Serial.printf("Sketch size: %u bytes\n", ESP.getSketchSize());
    Serial.printf("Free sketch space: %u bytes\n", ESP.getFreeSketchSpace());

    // Physical outputs are driven LOW before other modules initialize.
    outputsBegin();

    if (!sensorsBegin()) {
        Serial.println(F("ERROR: Sensor initialization failed; outputs remain disabled."));
    }

    if (!protectionBegin()) {
        Serial.println(F("ERROR: Protection storage initialization failed; outputs remain disabled."));
    }

    // Wi-Fi startup may block, but all outputs remain OFF during startup.
    if (!webBegin()) {
        Serial.println(F("ERROR: Web server initialization failed."));
    }

    // MQTT initializes after secrets and Wi-Fi setup; connection attempts are non-blocking in loop().
    mqttBegin();
}

void loop() {
    webHandleClient();
    mqttUpdate();

    static unsigned long lastSensorUpdate = 0;
    const unsigned long now = millis();
    if (now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL_MS) {
        lastSensorUpdate = now;
        sensorsUpdate();
    }

    protectionUpdate();
    webHandleClient();

    static unsigned long lastDebugTime = 0;
    const unsigned long debugNow = millis();
    if (debugNow - lastDebugTime >= 2000) {
        lastDebugTime = debugNow;
        debugPrintSensors();
        debugPrintOutputs();
    }
}
