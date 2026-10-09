#include "config.h"
#include "sensors.h"
#include "outputs.h"
#include "debug.h"
#include "web.h"

void setup() {
    debugBegin();
    debugPrintBanner();

    // Ініціалізація виходів:
    // реле мають залишатися вимкненими на старті.
    outputsBegin();

    // Ініціалізація датчиків.
    if (!sensorsBegin()) {
        Serial.println(F("ERROR: Sensor initialization failed!"));
    }

    // Запуск вебінтерфейсу.
    if (!webBegin()) {
        Serial.println(F("ERROR: Web server initialization failed!"));
    }
}

void loop() {
    webHandleClient();

    static unsigned long lastSensorUpdate = 0;
    const unsigned long now = millis();

    if (now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL_MS) {
        lastSensorUpdate = now;
        sensorsUpdate();
    }

    static unsigned long lastDebugTime = 0;
    const unsigned long debugNow = millis();

    if (debugNow - lastDebugTime >= 2000) {
        lastDebugTime = debugNow;
        debugPrintSensors();
        debugPrintOutputs();
    }
}