#pragma once

#include <Arduino.h>
#include <time.h>

void systemClockBegin();
void systemClockUpdate();
bool systemClockIsValid();
String systemClockGetLocalTime();
