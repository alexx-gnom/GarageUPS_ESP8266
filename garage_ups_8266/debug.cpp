#include "debug.h"

#include "sensors.h"
#include "outputs.h"

void debugBegin() {
    Serial.begin(115200);
    delay(100);
    Serial.println();
}

void debugPrintBanner() {
    Serial.println(F("================================"));
    Serial.println(F(" Garage UPS Controller"));
    Serial.println(F(" ESP8266 diagnostics"));
    Serial.println(F("================================"));
}

void debugPrintSensors() {
    Serial.println(F("--- Sensors ---"));

    Serial.print(F("Battery: "));
    Serial.print(sensorsBatteryVoltage(), 2);
    Serial.println(F(" V"));

    Serial.print(F("Charger: "));
    Serial.print(sensorsChargerVoltage(), 2);
    Serial.println(F(" V"));

    Serial.print(F("Current: "));
    Serial.print(sensorsCurrentAmps(), 2);
    Serial.println(F(" A"));

    Serial.print(F("NTC: "));
    Serial.print(sensorsNtcTemperature(), 2);
    Serial.println(F(" C"));

    Serial.print(F("DS18B20: "));
    Serial.print(sensorsDs18b20Temperature(), 2);
    Serial.println(F(" C"));

    Serial.println();
}

void debugPrintOutputs() {
    Serial.println(F("--- Outputs ---"));

    Serial.print(F("Safety relay: "));
    Serial.println(outputsGetSafety() ? F("ON") : F("OFF"));

    Serial.print(F("Load 0: "));
    Serial.println(outputsGetLoad0() ? F("ON") : F("OFF"));

    Serial.print(F("Load 1: "));
    Serial.println(outputsGetLoad1() ? F("ON") : F("OFF"));

    Serial.println();
}