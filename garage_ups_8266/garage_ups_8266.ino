
#include <Arduino.h>

#include "config.h"
#include "sensors.h"

unsigned long lastUpdate = 0;

void setup()
{
    Serial.begin(115200);
    delay(200);

    Serial.println();
    Serial.println("================================");
    Serial.println("Garage UPS Controller");
    Serial.println("Sensor module test");
    Serial.println("================================");

    if (!sensorsBegin())
    {
        Serial.println("ERROR: ADS1115 not found!");

        while (true)
            delay(1000);
    }

    Serial.println("ADS1115 OK");
    Serial.println("Sensors initialized");

    sensorsUpdate();
    printSensors();
    lastUpdate = millis();
}

void loop()
{
    if (millis() - lastUpdate >= SENSOR_UPDATE_INTERVAL_MS)
    {
        lastUpdate = millis();

        sensorsUpdate();
        printSensors();
    }
}

void printSensors()
{
    Serial.println();
    Serial.println("========== UPS SENSORS ==========");

    Serial.print("NTC pin   : ");
    Serial.print(sensorsNtcPinVoltage(), 4);
    Serial.println(" V");

    Serial.print("NTC R     : ");
    Serial.print(sensorsNtcResistance(), 0);
    Serial.println(" Ohm");

    Serial.print("T_NTC     : ");
    Serial.print(sensorsNtcTemperature(), 1);
    Serial.println(" C");

    Serial.println("---------------------------------");

    Serial.print("Charger   : ");
    Serial.print(sensorsChargerVoltage(), 2);
    Serial.println(" V");

    Serial.print("Battery   : ");
    Serial.print(sensorsBatteryVoltage(), 2);
    Serial.println(" V");

    Serial.println("---------------------------------");

    Serial.print("ACS pin   : ");
    Serial.print(sensorsCurrentPinVoltage(), 4);
    Serial.println(" V");

    Serial.print("ACS OUT   : ");
    Serial.print(sensorsCurrentOutputVoltage(), 3);
    Serial.println(" V");

    Serial.print("Current   : ");
    Serial.print(sensorsCurrentAmps(), 2);
    Serial.println(" A");

    Serial.println("---------------------------------");

    Serial.print("T_DS18B20 : ");
    Serial.print(sensorsDs18b20Temperature(), 2);
    Serial.println(" C");

    Serial.println("=================================");
}
