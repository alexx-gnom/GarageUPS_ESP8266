
#include "outputs.h"
#include "config.h"

static bool safetyState = false;
static bool load0State  = false;
static bool load1State  = false;

bool outputsBegin()
{
    digitalWrite(PIN_RELAY_SAFETY, HIGH);
    digitalWrite(PIN_RELAY_LOAD0, HIGH);
    digitalWrite(PIN_RELAY_LOAD1, HIGH);

    pinMode(PIN_RELAY_SAFETY, OUTPUT);
    pinMode(PIN_RELAY_LOAD0, OUTPUT);
    pinMode(PIN_RELAY_LOAD1, OUTPUT);

    safetyState = false;
    load0State  = false;
    load1State  = false;

    return true;
}

void outputsSetSafety(bool enabled)
{
    safetyState = enabled;
    digitalWrite(PIN_RELAY_SAFETY, enabled ? LOW : HIGH);
}

void outputsSetLoad0(bool enabled)
{
    load0State = enabled;
    digitalWrite(PIN_RELAY_LOAD0, enabled ? LOW : HIGH);
}

void outputsSetLoad1(bool enabled)
{
    load1State = enabled;
    digitalWrite(PIN_RELAY_LOAD1, enabled ? LOW : HIGH);
}

bool outputsGetSafety()
{
    return safetyState;
}

bool outputsGetLoad0()
{
    return load0State;
}

bool outputsGetLoad1()
{
    return load1State;
}
