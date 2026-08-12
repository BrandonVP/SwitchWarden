/*
===========================================================================
Name        : PowerControl.cpp
Author      : Brandon Van Pelt
Description : PSU hardware layer (see PowerControl.h).
===========================================================================
*/
#include "PowerControl.h"

// ---------------------------------------------------------------------------
// Pin map — J7 (Conn_01x10) on the Teensy 4.0 480x320 LCD board breaks out
// Teensy digital pins 0..8 (pin 10 of J7 is GND). Four pins start the supplies,
// four read their run status; pin 8 (J7.1) is left spare.
//
//   PSU          start out (J7 pin)     run monitor in (J7 pin)
//   PC 1         Teensy 0  (J7.9)       Teensy 4  (J7.5)
//   PC 2         Teensy 1  (J7.8)       Teensy 5  (J7.4)
//   PC 3         Teensy 2  (J7.7)       Teensy 6  (J7.3)
//   Cooling      Teensy 3  (J7.6)       Teensy 7  (J7.2)
//
// Adjust these to match the interface harness.
// ---------------------------------------------------------------------------
static const uint8_t START_PIN[PSU_COUNT]   = { 0, 1, 2, 3 };
static const uint8_t MONITOR_PIN[PSU_COUNT] = { 4, 5, 6, 7 };

// Logic polarity of the interface hardware. Flip if a supply starts on the
// opposite level, or reads "running" inverted.
static const bool START_ACTIVE_HIGH   = true;   // HIGH on START_PIN = commanded on
static const bool RUNNING_ACTIVE_HIGH = true;   // HIGH on MONITOR_PIN = running

// Teensy 4.0 pins are NOT 5V tolerant — the monitor inputs assume the interface
// hardware presents a 3.3V logic level. Use INPUT_PULLDOWN/PULLUP to suit it.
static const int MONITOR_MODE = INPUT;

static bool s_commanded[PSU_COUNT];

// Cooling is "auto" when it was forced on by a running PC (vs manually started).
// Only auto cooling gets shut off after the cooldown; manual cooling stays on.
static bool     s_coolingAuto = false;
static bool     s_coolingOffPending = false;
static uint32_t s_coolingOffAtMs = 0;

static void driveStart(uint8_t psu, bool on)
{
    digitalWrite(START_PIN[psu], (on == START_ACTIVE_HIGH) ? HIGH : LOW);
}

void POWER_init()
{
    for (uint8_t p = 0; p < PSU_COUNT; p++)
    {
        s_commanded[p] = false;
        pinMode(START_PIN[p], OUTPUT);
        driveStart(p, false);            // everything off at boot
        pinMode(MONITOR_PIN[p], MONITOR_MODE);
    }

    s_coolingAuto = false;
    s_coolingOffPending = false;
}

bool POWER_anyPcOn()
{
    return s_commanded[PSU_PC1] || s_commanded[PSU_PC2] || s_commanded[PSU_PC3];
}

bool POWER_isCoolingLocked()
{
    return POWER_anyPcOn();
}

void POWER_setCommanded(uint8_t psu, bool on)
{
    if (psu >= PSU_COUNT)
        return;

    // Safety: cooling cannot be switched off while a PC is running.
    if (psu == PSU_COOLING && !on && POWER_anyPcOn())
        return;

    s_commanded[psu] = on;
    driveStart(psu, on);

    if (psu == PSU_COOLING)
    {
        // Direct user control of cooling — clear the auto flag and any pending
        // auto-off so it stays exactly as commanded.
        s_coolingAuto = false;
        s_coolingOffPending = false;
    }
    else if (POWER_anyPcOn())
    {
        // A PC is running: force the cooling supply on and mark it auto-driven.
        if (!s_commanded[PSU_COOLING])
        {
            s_commanded[PSU_COOLING] = true;
            driveStart(PSU_COOLING, true);
        }
        s_coolingAuto = true;
        s_coolingOffPending = false;
    }
    else if (s_commanded[PSU_COOLING] && s_coolingAuto)
    {
        // Last PC just turned off and cooling is running because we forced it —
        // schedule the auto-off after the cooldown.
        s_coolingOffPending = true;
        s_coolingOffAtMs = millis() + COOLING_OFF_DELAY_MS;
    }
}

void POWER_tick()
{
    if (!s_coolingOffPending)
        return;

    // Signed compare so millis() rollover is handled.
    if ((int32_t)(millis() - s_coolingOffAtMs) < 0)
        return;

    s_coolingOffPending = false;

    // Only shut cooling down if it is still idle and still auto-driven — a PC
    // may have restarted, or the user may have taken manual control, during the
    // cooldown.
    if (!POWER_anyPcOn() && s_commanded[PSU_COOLING] && s_coolingAuto)
    {
        s_commanded[PSU_COOLING] = false;
        driveStart(PSU_COOLING, false);
        s_coolingAuto = false;
    }
}

void POWER_toggle(uint8_t psu)
{
    if (psu < PSU_COUNT)
        POWER_setCommanded(psu, !s_commanded[psu]);
}

bool POWER_isCommanded(uint8_t psu)
{
    return psu < PSU_COUNT && s_commanded[psu];
}

bool POWER_isRunning(uint8_t psu)
{
    if (psu >= PSU_COUNT)
        return false;

    int level = digitalRead(MONITOR_PIN[psu]);
    return RUNNING_ACTIVE_HIGH ? (level == HIGH) : (level == LOW);
}
