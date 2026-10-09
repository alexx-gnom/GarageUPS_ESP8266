
#pragma once

#include <Arduino.h>

bool outputsBegin();

void outputsSetSafety(bool enabled);
void outputsSetLoad0(bool enabled);
void outputsSetLoad1(bool enabled);

bool outputsGetSafety();
bool outputsGetLoad0();
bool outputsGetLoad1();
