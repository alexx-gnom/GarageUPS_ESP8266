#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- Pins ----------
#define SDA_PIN 4       // GPIO4
#define SCL_PIN 5       // GPIO5
#define ONE_WIRE_PIN 13 // GPIO13

// ---------- ADC ----------
Adafruit_ADS1115 ads;

// ---------- DS18B20 ----------
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// ---------- Resistor dividers ----------
const float BAT_R_TOP = 68000.0;
const float BAT_R_BOTTOM = 10000.0;

const float ACS_R_TOP = 10000.0;
const float ACS_R_BOTTOM = 2200.0;

// ACS712-30A sensitivity
const float ACS_SENSITIVITY = 0.066; // V/A

// ---------- ACS zero ----------
float acsZero = 2.5;

void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("       UPS SENSOR TEST");
  Serial.println("================================");

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // ADS1115
  if (!ads.begin())
  {
    Serial.println("ERROR: ADS1115 not found!");
    while (1)
    {
      delay(1000);
    }
  }

  Serial.println("ADS1115: OK");

  // Gain = +/-4.096 V
  ads.setGain(GAIN_ONE);

  // DS18B20
  sensors.begin();

  Serial.print("DS18B20 sensors: ");
  Serial.println(sensors.getDeviceCount());

  Serial.println();
  Serial.println("Starting measurements...");
  Serial.println();
}

void loop()
{
  // ---------- ADS1115 ----------
  int16_t rawA0 = ads.readADC_SingleEnded(0);
  int16_t rawA1 = ads.readADC_SingleEnded(1);

  float voltageA0 = ads.computeVolts(rawA0);
  float voltageA1 = ads.computeVolts(rawA1);

  // Battery voltage
  float batteryVoltage =
      voltageA0 * (BAT_R_TOP + BAT_R_BOTTOM) / BAT_R_BOTTOM;

  // ACS voltage
  float acsVoltage =
      voltageA1 * (ACS_R_TOP + ACS_R_BOTTOM) / ACS_R_BOTTOM;

  // Current
  float current =
      (acsVoltage - acsZero) / ACS_SENSITIVITY;

  // ---------- DS18B20 ----------
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // ---------- Serial ----------
  Serial.println("--------------------------------");

  Serial.print("A0 raw:       ");
  Serial.println(rawA0);

  Serial.print("A0 voltage:   ");
  Serial.print(voltageA0, 4);
  Serial.println(" V");

  Serial.print("BATTERY:      ");
  Serial.print(batteryVoltage, 3);
  Serial.println(" V");

  Serial.println();

  Serial.print("A1 raw:       ");
  Serial.println(rawA1);

  Serial.print("A1 voltage:   ");
  Serial.print(voltageA1, 4);
  Serial.println(" V");

  Serial.print("ACS OUT:      ");
  Serial.print(acsVoltage, 3);
  Serial.println(" V");

  Serial.print("CURRENT:      ");
  Serial.print(current, 3);
  Serial.println(" A");

  Serial.println();

  Serial.print("TEMPERATURE:  ");
  Serial.print(temperature, 2);
  Serial.println(" C");

  delay(1000);
}