#pragma once

#include <Arduino.h>

void telegramBegin();
void telegramUpdate();
bool telegramIsConfigured();
bool telegramRequestStatusReport(const char* source);
