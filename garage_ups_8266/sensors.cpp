
#include "sensors.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// =====================================================
// Hardware objects
// =====================================================

static Adafruit_ADS1115 ads;

static OneWire oneWire(PIN_DS18B20);
static DallasTemperature ds18(&oneWire);

static bool adsReady = false;
static unsigned long lastAdsInitAttempt = 0;
static constexpr unsigned long ADS_RETRY_INTERVAL_MS = 5000;
static bool ntcMeasurementValid = false;
static unsigned long lastNtcMeasurementAt = 0;
static constexpr unsigned long NTC_MAX_AGE_MS = 3000;
static bool dsConversionPending = false;
static unsigned long dsConversionStartedAt = 0;
static constexpr unsigned long DS_CONVERSION_TIME_MS = 750;

// =====================================================
// Cached measurements
// =====================================================

static float ntcPinV = 0.0f;
static float ntcResistance = NAN;
static float ntcTemperature = NAN;

static float chargerPinV = 0.0f;
static float chargerVoltage = 0.0f;

static float batteryPinV = 0.0f;
static float batteryVoltage = 0.0f;

static float currentPinV = 0.0f;
static float currentOutputV = 0.0f;
static float currentAmps = 0.0f;

static float dsTemperature = DEVICE_DISCONNECTED_C;

// =====================================================
// Read ADC channel with averaging
// =====================================================

static float readAdcVoltage(uint8_t channel)
{
    long sum = 0;

    for (uint8_t i = 0; i < ADC_AVERAGE_SAMPLES; i++)
    {
        sum += ads.readADC_SingleEnded(channel);
        delay(ADC_SAMPLE_DELAY_MS);
    }

    float raw = sum / static_cast<float>(ADC_AVERAGE_SAMPLES);

    return ads.computeVolts(static_cast<int16_t>(raw));
}

// =====================================================
// Voltage divider conversion
// =====================================================

static float dividerInputVoltage(
    float adcVoltage,
    float rTop,
    float rBottom)
{
    return adcVoltage * (rTop + rBottom) / rBottom;
}

// =====================================================
// NTC resistance
// 3.3V -> NTC -> ADC -> 11k -> GND
// =====================================================

static float calculateNtcResistance(float voltage)
{
    if (voltage <= 0.001f)
        return INFINITY;

    if (voltage >= 3.299f)
        return 0.0f;

    return NTC_R_FIXED * (3.3f / voltage - 1.0f);
}

// =====================================================
// NTC temperature, Beta equation
// =====================================================

static float calculateNtcTemperature(float resistance)
{
    if (!isfinite(resistance) || resistance <= 0.0f)
        return NAN;

    const float t25 = 25.0f + 273.15f;

    const float tempK = 1.0f /
        (1.0f / t25 +
         log(resistance / NTC_R25) / NTC_BETA);

    return tempK - 273.15f;
}

// =====================================================
// Initialization
// =====================================================

bool sensorsBegin()
{
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    adsReady = ads.begin();
    lastAdsInitAttempt = millis();

    if (adsReady) {
        ads.setGain(GAIN_ONE);
        Serial.println(F("ADS1115 initialized."));
    } else {
        Serial.println(F("ADS1115 unavailable; will retry every 5 seconds."));
    }

    // DS18B20 is initialized independently; its failure must not affect ADS/NTC.
    ds18.begin();
    ds18.setWaitForConversion(false);
    ds18.requestTemperatures();
    dsConversionStartedAt = millis();
    dsConversionPending = true;

    return adsReady;
}

// =====================================================
// Update all measurements
// =====================================================

void sensorsUpdate()
{
    const unsigned long now = millis();

    // Recover ADS1115 without rebooting the ESP. Keep retry attempts spaced apart.
    if (!adsReady && now - lastAdsInitAttempt >= ADS_RETRY_INTERVAL_MS) {
        lastAdsInitAttempt = now;
        adsReady = ads.begin();
        if (adsReady) {
            ads.setGain(GAIN_ONE);
            Serial.println(F("ADS1115 reinitialized successfully."));
        } else {
            Serial.println(F("ADS1115 retry failed; will try again."));
        }
    }

    // NTC is read first and independently of DS18B20 processing.
    if (adsReady) {
        ntcPinV = readAdcVoltage(ADC_NTC);
        ntcResistance = calculateNtcResistance(ntcPinV);
        ntcTemperature = calculateNtcTemperature(ntcResistance);
        ntcMeasurementValid = isfinite(ntcTemperature) && ntcTemperature >= -40.0f && ntcTemperature <= 125.0f;
        lastNtcMeasurementAt = millis();

        // Other ADC channels are non-temperature telemetry.
        chargerPinV = readAdcVoltage(ADC_CHARGER);
        chargerVoltage = dividerInputVoltage(chargerPinV, CHARGER_R_TOP, CHARGER_R_BOTTOM);

        batteryPinV = readAdcVoltage(ADC_BATTERY);
        batteryVoltage = dividerInputVoltage(batteryPinV, BAT_R_TOP, BAT_R_BOTTOM);

        currentPinV = readAdcVoltage(ADC_CURRENT);
        currentOutputV = dividerInputVoltage(currentPinV, ACS_R_TOP, ACS_R_BOTTOM);
        currentAmps = (currentPinV - ACS_ZERO_PIN) / ACS_SENSITIVITY_PIN;
        if (fabs(currentAmps) < 0.15f) currentAmps = 0.0f;
    } else {
        ntcMeasurementValid = false;
    }

    // DS18B20: asynchronous conversion; independent of ADS availability.
    if (dsConversionPending && millis() - dsConversionStartedAt >= DS_CONVERSION_TIME_MS) {
        const float temperature = ds18.getTempCByIndex(0);
        if (temperature != DEVICE_DISCONNECTED_C) {
            dsTemperature = temperature;
        }
        ds18.requestTemperatures();
        dsConversionStartedAt = millis();
        dsConversionPending = true;
    }
}

// =====================================================
// Getters
// =====================================================

float sensorsBatteryVoltage()
{
    return batteryVoltage;
}

float sensorsChargerVoltage()
{
    return chargerVoltage;
}

float sensorsCurrentPinVoltage()
{
    return currentPinV;
}

float sensorsCurrentOutputVoltage()
{
    return currentOutputV;
}

float sensorsCurrentAmps()
{
    return currentAmps;
}

float sensorsNtcPinVoltage()
{
    return ntcPinV;
}

float sensorsNtcResistance()
{
    return ntcResistance;
}

float sensorsNtcTemperature()
{
    return ntcTemperature;
}

bool sensorsNtcMeasurementOk()
{
    return ntcMeasurementValid && (millis() - lastNtcMeasurementAt <= NTC_MAX_AGE_MS);
}

float sensorsDs18b20Temperature()
{
    return dsTemperature;
}
