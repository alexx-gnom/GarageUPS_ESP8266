
#pragma once

// =====================================================
// Garage UPS Controller — configuration
// ESP8266 Witty / ESP-12F
// =====================================================

// --------------------- I2C ----------------------------

constexpr uint8_t PIN_I2C_SDA = 4;  // GPIO4
constexpr uint8_t PIN_I2C_SCL = 5;  // GPIO5

// --------------------- Sensors -----------------------

constexpr uint8_t PIN_DS18B20 = 13; // GPIO13

// --------------------- Outputs -----------------------

constexpr uint8_t PIN_RELAY_SAFETY = 12; // GPIO12
constexpr uint8_t PIN_RELAY_LOAD0  = 14; // GPIO14
constexpr uint8_t PIN_RELAY_LOAD1  = 16; // GPIO16

// --------------------- ADS1115 -----------------------

constexpr uint8_t ADC_NTC     = 0; // A0
constexpr uint8_t ADC_CHARGER = 1; // A1
constexpr uint8_t ADC_BATTERY = 2; // A2
constexpr uint8_t ADC_CURRENT = 3; // A3

// --------------------- NTC ---------------------------

// Divider: 3.3V -> NTC -> A0 -> fixed resistor -> GND

constexpr float NTC_R_FIXED = 11000.0f;
constexpr float NTC_R25     = 10000.0f;
constexpr float NTC_BETA    = 3950.0f;

// --------------------- Voltage dividers ---------------

// Charger: 19V -> 100k -> A1 -> 11k -> GND
constexpr float CHARGER_R_TOP    = 100000.0f;
constexpr float CHARGER_R_BOTTOM = 11000.0f;

// Battery: BAT+ -> 100k -> A2 -> 11k -> GND
constexpr float BAT_R_TOP    = 100000.0f;
constexpr float BAT_R_BOTTOM = 11000.0f;

// --------------------- ACS712 -------------------------

// ACS OUT -> 9.889k -> A3 -> 3.3k -> GND
// Calibrated values measured at ADS1115 input.

constexpr float ACS_R_TOP    = 9889.0f;
constexpr float ACS_R_BOTTOM = 3300.0f;

constexpr float ACS_ZERO_PIN        = 0.6320f;
constexpr float ACS_SENSITIVITY_PIN = 0.01497f; // V/A

// --------------------- ADS1115 ------------------------

constexpr uint8_t ADC_AVERAGE_SAMPLES = 4;
constexpr uint8_t ADC_SAMPLE_DELAY_MS = 0;

// --------------------- Timing -------------------------

constexpr unsigned long SENSOR_UPDATE_INTERVAL_MS = 1000;

// --------------------- Protection -------------------------

constexpr float BAT_OVERHEAT_TEMPERATURE = 40.0f;

constexpr float _BAT_LOW_VOLTAGE = 11.2f;

// Charger-source presence threshold, based on the currently observed ~12.84 V.
constexpr float CHARGER_PRESENT_VOLTAGE = 12.0f;
