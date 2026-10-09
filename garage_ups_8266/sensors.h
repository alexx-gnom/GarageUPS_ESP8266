
#pragma once

#include <Arduino.h>

// Ініціалізація ADS1115 та DS18B20.
bool sensorsBegin();

// Оновити всі вимірювання.
void sensorsUpdate();

// Напруга на вході акумулятора, V.
float sensorsBatteryVoltage();

// Напруга на вході живлення зарядки, V.
float sensorsChargerVoltage();

// Напруга на вході ADS1115 від ACS712, V.
float sensorsCurrentPinVoltage();

// Відновлена напруга виходу ACS712 до дільника, V.
float sensorsCurrentOutputVoltage();

// Розрахований струм, A.
float sensorsCurrentAmps();

// Напруга на вході NTC, V.
float sensorsNtcPinVoltage();

// Розрахований опір NTC, Ohm.
float sensorsNtcResistance();

// Розрахована температура NTC, °C.
float sensorsNtcTemperature();

// Температура DS18B20, °C.
float sensorsDs18b20Temperature();
