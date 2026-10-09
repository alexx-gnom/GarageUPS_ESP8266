#pragma once

#include <Arduino.h>

bool protectionBegin();
void protectionUpdate();

bool protectionIsOverheatLatched();
bool protectionIsLowBatteryCutoff();
bool protectionTemperatureSensorsOk();
bool protectionAllowsLoads();
bool protectionIsChargerPresent();
bool protectionBatteryMeasurementOk();

bool protectionGetLoad0Desired();
bool protectionGetLoad1Desired();
bool protectionGetChargerRequest();
bool protectionIsLoad0Restarting();
bool protectionIsLoad1Restarting();

bool protectionSetChargerRequest(bool enabled);
bool protectionToggleLoadDesired(uint8_t channel);
bool protectionRestartLoad(uint8_t channel);
