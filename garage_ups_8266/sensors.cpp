
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

    if (!adsReady)
        return false;

    ads.setGain(GAIN_ONE);

    ds18.begin();

    return true;
}

// =====================================================
// Update all measurements
// =====================================================

void sensorsUpdate()
{
    if (!adsReady)
        return;

    // NTC
    ntcPinV = readAdcVoltage(ADC_NTC);
    ntcResistance = calculateNtcResistance(ntcPinV);
    ntcTemperature = calculateNtcTemperature(ntcResistance);

    // Charger voltage
    chargerPinV = readAdcVoltage(ADC_CHARGER);
    chargerVoltage = dividerInputVoltage(
        chargerPinV,
        CHARGER_R_TOP,
        CHARGER_R_BOTTOM);

    // Battery voltage
    batteryPinV = readAdcVoltage(ADC_BATTERY);
    batteryVoltage = dividerInputVoltage(
        batteryPinV,
        BAT_R_TOP,
        BAT_R_BOTTOM);

    // ACS712
    currentPinV = readAdcVoltage(ADC_CURRENT);

    currentOutputV = dividerInputVoltage(
        currentPinV,
        ACS_R_TOP,
        ACS_R_BOTTOM);

    currentAmps =
        (currentPinV - ACS_ZERO_PIN) /
        ACS_SENSITIVITY_PIN;

    if (fabs(currentAmps) < 0.15f)
        currentAmps = 0.0f;

    // DS18B20
    ds18.requestTemperatures();
    dsTemperature = ds18.getTempCByIndex(0);
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

float sensorsDs18b20Temperature()
{
    return dsTemperature;
}
