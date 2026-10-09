
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// =====================================================
// UPS Sensor Test v0.2
// ESP8266 Witty + ADS1115 + DS18B20
// =====================================================

// GPIO
#define PIN_SDA       4
#define PIN_SCL       5
#define PIN_DS18B20   13

// ADS1115 channels
#define ADC_NTC       0
#define ADC_CHARGER   1
#define ADC_BATTERY   2
#define ADC_ACS       3

Adafruit_ADS1115 ads;

OneWire oneWire(PIN_DS18B20);
DallasTemperature ds18(&oneWire);

// =====================================================
// Divider resistors (Ohms)
// =====================================================

// A0: 3.3V -> NTC -> A0 -> 11k -> GND
const float NTC_R_FIXED = 11000.0;
const float NTC_R25     = 10000.0;
const float NTC_BETA    = 3950.0;  // Тимчасово, калібруємо за DS18B20

// A1: 19V -> 100k -> A1 -> 11k -> GND
const float CHARGER_R_TOP    = 100000.0;
const float CHARGER_R_BOTTOM = 11000.0;

// A2: BAT+ -> 100k -> A2 -> 11k -> GND
const float BAT_R_TOP    = 100000.0;
const float BAT_R_BOTTOM = 11000.0;

// A3: ACS OUT -> 9889 Ohm -> A3 -> 3300 Ohm -> GND
const float ACS_R_TOP    = 9889.0;
const float ACS_R_BOTTOM = 3300.0;

// =====================================================
// ACS712 calibration
// Zero measured at ADS input with no current.
// Sensitivity calibrated against bench PSU at 1.77 A.
// =====================================================

const float ACS_ZERO_PIN = 0.6320;
const float ACS_SENSITIVITY_PIN = 0.01497; // V/A at ADS pin

// =====================================================
// ADC reading with averaging
// =====================================================

float readADCVoltage(uint8_t channel)
{
  const int samples = 16;
  long sum = 0;

  for (int i = 0; i < samples; i++)
  {
    sum += ads.readADC_SingleEnded(channel);
    delay(2);
  }

  float raw = sum / (float)samples;
  return ads.computeVolts((int16_t)raw);
}

// =====================================================
// Divider voltage -> original input voltage
// =====================================================

float dividerInputVoltage(
    float adcVoltage,
    float rTop,
    float rBottom)
{
  return adcVoltage *
         (rTop + rBottom) / rBottom;
}

// =====================================================
// NTC calculation
// 3.3V -> NTC -> ADC -> 11k -> GND
// =====================================================

float calculateNTCResistance(float voltage)
{
  if (voltage <= 0.001)
    return INFINITY;

  if (voltage >= 3.299)
    return 0.0;

  return NTC_R_FIXED * (3.3 / voltage - 1.0);
}

float calculateNTCTemperature(float resistance)
{
  if (!isfinite(resistance) || resistance <= 0)
    return NAN;

  const float t25 = 25.0 + 273.15;

  float tempK = 1.0 /
      (1.0 / t25 +
       log(resistance / NTC_R25) / NTC_BETA);

  return tempK - 273.15;
}

// =====================================================
// Print all measurements
// =====================================================

void printSensors()
{
  float ntcV     = readADCVoltage(ADC_NTC);
  float chargerV = readADCVoltage(ADC_CHARGER);
  float batteryV = readADCVoltage(ADC_BATTERY);
  float acsV     = readADCVoltage(ADC_ACS);

  float ntcR = calculateNTCResistance(ntcV);
  float ntcT = calculateNTCTemperature(ntcR);

  float chargerInput = dividerInputVoltage(
      chargerV, CHARGER_R_TOP, CHARGER_R_BOTTOM);

  float batteryInput = dividerInputVoltage(
      batteryV, BAT_R_TOP, BAT_R_BOTTOM);

  float acsOutV = dividerInputVoltage(
      acsV, ACS_R_TOP, ACS_R_BOTTOM);

  float currentA =
      (acsV - ACS_ZERO_PIN) /
      ACS_SENSITIVITY_PIN;

  if (fabs(currentA) < 0.15)
    currentA = 0.0;

  float dsT = ds18.getTempCByIndex(0);

  Serial.println();
  Serial.println("========== SENSOR TEST v0.2 ==========");

  Serial.print("A0 NTC pin : ");
  Serial.print(ntcV, 4);
  Serial.println(" V");

  Serial.print("NTC R      : ");
  if (isfinite(ntcR))
    Serial.print(ntcR, 0);
  else
    Serial.print("OPEN");
  Serial.println(" Ohm");

  Serial.print("T_NTC      : ");
  if (isfinite(ntcT))
    Serial.print(ntcT, 1);
  else
    Serial.print("INVALID");
  Serial.println(" C");

  Serial.println("--------------------------------------");

  Serial.print("A1 19V pin : ");
  Serial.print(chargerV, 4);
  Serial.println(" V");

  Serial.print("Charger in : ");
  Serial.print(chargerInput, 2);
  Serial.println(" V");

  Serial.println("--------------------------------------");

  Serial.print("A2 BAT pin : ");
  Serial.print(batteryV, 4);
  Serial.println(" V");

  Serial.print("Battery    : ");
  Serial.print(batteryInput, 2);
  Serial.println(" V");

  Serial.println("--------------------------------------");

  Serial.print("A3 ACS pin : ");
  Serial.print(acsV, 4);
  Serial.println(" V");

  Serial.print("ACS OUT est: ");
  Serial.print(acsOutV, 3);
  Serial.println(" V");

  Serial.print("ACS zero   : ");
  Serial.print(ACS_ZERO_PIN, 4);
  Serial.println(" V");

  Serial.print("Current    : ");
  Serial.print(currentA, 2);
  Serial.println(" A");

  Serial.println("--------------------------------------");

  Serial.print("T_DS18B20  : ");
  if (dsT == DEVICE_DISCONNECTED_C)
    Serial.println("SENSOR ERROR");
  else
  {
    Serial.print(dsT, 2);
    Serial.println(" C");
  }

  Serial.println("======================================");
}

// =====================================================
// Setup
// =====================================================

void setup()
{
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("UPS SENSOR TEST v0.2");

  Wire.begin(PIN_SDA, PIN_SCL);

  if (!ads.begin())
  {
    Serial.println("ERROR: ADS1115 not found!");
    while (true)
      delay(1000);
  }

  ads.setGain(GAIN_ONE);

  ds18.begin();

  Serial.println("ADS1115 OK");
  Serial.println("DS18B20 initialized");
  Serial.println("ACS zero is fixed; no startup calibration.");
}

// =====================================================
// Loop
// =====================================================

void loop()
{
  ds18.requestTemperatures();
  printSensors();

  delay(1000);
}
