#include "protection.h"

#include <EEPROM.h>
#include <math.h>

#include "config.h"
#include "sensors.h"
#include "outputs.h"

// GPIO0 is the board's FLASH button (pressed = LOW).
static constexpr uint8_t PIN_BUTTON_FLASH = 0;

// EEPROM emulation: write only when persistent state changes.
static constexpr size_t EEPROM_SIZE = 8;
static constexpr uint8_t EEPROM_INIT_ADDR = 0;
static constexpr uint8_t EEPROM_OVERHEAT_ADDR = 1;
static constexpr uint8_t EEPROM_LOAD0_ADDR = 2;
static constexpr uint8_t EEPROM_LOAD1_ADDR = 3;
static constexpr uint8_t EEPROM_INIT_MARKER = 0xA7;

static constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;
static constexpr unsigned long LOAD_RESTART_DELAY_MS = 7000;

static bool storageReady = false;
static bool overheatLatched = true; // Fail-safe until EEPROM state is read.
static bool load0Desired = false;
static bool load1Desired = false;
static bool chargerRequest = true;  // Volatile: defaults ON after every boot.
static bool lowBatteryCutoff = false;
static bool temperatureSensorsOk = false;
static bool batteryMeasurementOk = false;
static bool chargerMeasurementOk = false;
static bool chargerPresent = false;

static bool buttonLastReading = HIGH;
static bool buttonStableState = HIGH;
static unsigned long buttonLastChange = 0;

static bool load0RestartPending = false;
static bool load1RestartPending = false;
static unsigned long load0RestartStartedAt = 0;
static unsigned long load1RestartStartedAt = 0;

static bool validTemperature(float t) {
    return isfinite(t) && t >= -40.0f && t <= 125.0f;
}

static void disableAllOutputs() {
    outputsSetSafety(false);
    outputsSetLoad0(false);
    outputsSetLoad1(false);
}

static bool writePersistentState(bool newOverheat, bool newLoad0, bool newLoad1) {
    if (!storageReady) return false;

    EEPROM.write(EEPROM_INIT_ADDR, EEPROM_INIT_MARKER);
    EEPROM.write(EEPROM_OVERHEAT_ADDR, newOverheat ? 1 : 0);
    EEPROM.write(EEPROM_LOAD0_ADDR, newLoad0 ? 1 : 0);
    EEPROM.write(EEPROM_LOAD1_ADDR, newLoad1 ? 1 : 0);

    if (EEPROM.commit()) return true;

    // If persistence fails, do not continue operating with uncertain state.
    storageReady = false;
    overheatLatched = true;
    disableAllOutputs();
    return false;
}

static bool saveOverheat(bool latched) {
    return writePersistentState(latched, load0Desired, load1Desired);
}

static bool buttonPressed() {
    const bool reading = digitalRead(PIN_BUTTON_FLASH);
    const unsigned long now = millis();

    if (reading != buttonLastReading) {
        buttonLastReading = reading;
        buttonLastChange = now;
    }

    if (now - buttonLastChange >= BUTTON_DEBOUNCE_MS &&
        reading != buttonStableState) {
        buttonStableState = reading;
        return buttonStableState == LOW;
    }
    return false;
}

static void updateRestartTimers() {
    const unsigned long now = millis();
    if (load0RestartPending &&
        now - load0RestartStartedAt >= LOAD_RESTART_DELAY_MS) {
        load0RestartPending = false;
    }
    if (load1RestartPending &&
        now - load1RestartStartedAt >= LOAD_RESTART_DELAY_MS) {
        load1RestartPending = false;
    }
}

static void applyOutputs() {
    // Thermal safety and trustworthy persistence/sensor readings always win.
    if (!storageReady || overheatLatched || !temperatureSensorsOk) {
        disableAllOutputs();
        return;
    }

    // Charger output is enabled by default at boot, unless temporarily overridden.
    outputsSetSafety(chargerRequest);

    // On charger supply, honor the EEPROM preferences regardless of battery voltage.
    // On battery only, require a valid battery reading and no latched low-voltage cutoff.
    const bool loadsAllowed =
        chargerPresent || (batteryMeasurementOk && !lowBatteryCutoff);

    outputsSetLoad0(loadsAllowed && load0Desired && !load0RestartPending);
    outputsSetLoad1(loadsAllowed && load1Desired && !load1RestartPending);
}

bool protectionBegin() {
    pinMode(PIN_BUTTON_FLASH, INPUT_PULLUP);

    EEPROM.begin(EEPROM_SIZE);
    storageReady = true;

    const uint8_t marker = EEPROM.read(EEPROM_INIT_ADDR);

    if (marker == EEPROM_INIT_MARKER) {
        // Any value other than explicit zero is treated as a latched fault.
        overheatLatched = EEPROM.read(EEPROM_OVERHEAT_ADDR) != 0;

        const uint8_t savedLoad0 = EEPROM.read(EEPROM_LOAD0_ADDR);
        const uint8_t savedLoad1 = EEPROM.read(EEPROM_LOAD1_ADDR);

        // Accept only explicit 0/1 values. Legacy/uninitialized bytes migrate to OFF.
        load0Desired = savedLoad0 == 1;
        load1Desired = savedLoad1 == 1;

        if (savedLoad0 > 1 || savedLoad1 > 1) {
            if (!writePersistentState(overheatLatched, load0Desired, load1Desired)) {
                disableAllOutputs();
                return false;
            }
        }
    } else {
        // First initialization: fail-safe defaults, all load preferences OFF.
        overheatLatched = false;
        load0Desired = false;
        load1Desired = false;

        if (!writePersistentState(false, false, false)) {
            overheatLatched = true;
            disableAllOutputs();
            return false;
        }
    }

    // Actual outputs remain OFF until the first valid sensor update passes protections.
    disableAllOutputs();
    return true;
}

void protectionUpdate() {
    updateRestartTimers();

    const float ntc = sensorsNtcTemperature();
    const float ds18 = sensorsDs18b20Temperature();
    const bool ntcOk = validTemperature(ntc);
    const bool ds18Ok = validTemperature(ds18);
    temperatureSensorsOk = ntcOk && ds18Ok;

    // A valid sensor reaching the limit latches and persists the thermal fault.
    if ((ntcOk && ntc >= BAT_OVERHEAT_TEMPERATURE) ||
        (ds18Ok && ds18 >= BAT_OVERHEAT_TEMPERATURE)) {
        if (!overheatLatched) {
            overheatLatched = true; // Latch in RAM before attempting EEPROM write.
            saveOverheat(true);
        }
    }

    // Physical FLASH button clears the latch only if both sensors are valid and cool.
    if (buttonPressed() && overheatLatched && temperatureSensorsOk &&
        ntc < BAT_OVERHEAT_TEMPERATURE &&
        ds18 < BAT_OVERHEAT_TEMPERATURE && storageReady) {
        if (saveOverheat(false)) overheatLatched = false;
    }

    const float batteryV = sensorsBatteryVoltage();
    const float chargerV = sensorsChargerVoltage();

    batteryMeasurementOk =
        isfinite(batteryV) && batteryV > 1.0f && batteryV < 20.0f;
    chargerMeasurementOk =
        isfinite(chargerV) && chargerV >= 0.0f && chargerV < 30.0f;

    chargerPresent =
        chargerMeasurementOk && chargerV >= CHARGER_PRESENT_VOLTAGE;

    // Low-voltage protection applies only while running from battery.
    // Charger appearance clears the latch. On ESP restart the RAM latch starts clear,
    // but this check immediately re-applies cutoff if battery is still below threshold.
    if (chargerPresent) {
        lowBatteryCutoff = false;
    } else if (batteryMeasurementOk && batteryV < _BAT_LOW_VOLTAGE) {
        lowBatteryCutoff = true;
    }

    applyOutputs();
}

bool protectionIsOverheatLatched() { return overheatLatched; }
bool protectionIsLowBatteryCutoff() { return lowBatteryCutoff; }
bool protectionTemperatureSensorsOk() { return temperatureSensorsOk; }
bool protectionIsChargerPresent() { return chargerPresent; }
bool protectionBatteryMeasurementOk() { return batteryMeasurementOk; }

bool protectionAllowsLoads() {
    return storageReady && !overheatLatched && temperatureSensorsOk &&
           (chargerPresent || (batteryMeasurementOk && !lowBatteryCutoff));
}

bool protectionGetLoad0Desired() { return load0Desired; }
bool protectionGetLoad1Desired() { return load1Desired; }
bool protectionGetChargerRequest() { return chargerRequest; }
bool protectionIsLoad0Restarting() { return load0RestartPending; }
bool protectionIsLoad1Restarting() { return load1RestartPending; }

bool protectionSetChargerRequest(bool enabled) {
    chargerRequest = enabled; // RAM-only override; reset to ON after reboot.
    applyOutputs();
    return true;
}

bool protectionToggleLoadDesired(uint8_t channel) {
    if (!storageReady || (channel != 0 && channel != 1)) return false;

    const bool old0 = load0Desired;
    const bool old1 = load1Desired;

    if (channel == 0) load0Desired = !load0Desired;
    else load1Desired = !load1Desired;

    if (!writePersistentState(overheatLatched, load0Desired, load1Desired)) {
        load0Desired = old0;
        load1Desired = old1;
        disableAllOutputs();
        return false;
    }

    applyOutputs();
    return true;
}

bool protectionRestartLoad(uint8_t channel) {
    if (channel == 0) {
        load0RestartPending = true;
        load0RestartStartedAt = millis();
        outputsSetLoad0(false);
        return true;
    }

    if (channel == 1) {
        load1RestartPending = true;
        load1RestartStartedAt = millis();
        outputsSetLoad1(false);
        return true;
    }

    return false;
}
