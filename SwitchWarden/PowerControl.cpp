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
// The opto isolation module is inverting: it pulls its output LOW when its input
// (the PSU's +12V) is present, and idles HIGH. So a LOW on MONITOR_PIN = running.
static const bool RUNNING_ACTIVE_HIGH = false;

// Teensy 4.0 pins are NOT 5V tolerant. Power the opto's OUTPUT side from 3.3V so
// its HIGH is a safe 3.3V; it idles high, and the pull-up keeps it defined.
static const int MONITOR_MODE = INPUT_PULLUP;

// Drive style per channel. Cooling is a LEVEL control — hold PS_ON asserted to
// keep the supply on. Each PC is a MOMENTARY motherboard power button (PWR_SW):
// a short press toggles it, a long hold force-powers-off.
static const bool CH_IS_PULSE[PSU_COUNT] = { true, true, true, false };

#define PWR_PULSE_SHORT_MS    300U    // a normal power-button press
#define PWR_PULSE_LONG_MS     6000U   // >4s hold = ATX hard power-off
#define PWR_PULSE_COOLDOWN_MS 4000U   // ignore repeat short presses briefly after one

static bool s_commanded[PSU_COUNT];

// Cooling is "auto" when it was forced on by a running PC (vs manually started).
// Only auto cooling gets shut off after the cooldown; manual cooling stays on.
static bool     s_coolingAuto = false;
static bool     s_coolingOffPending = false;
static uint32_t s_coolingOffAtMs = 0;

// Cooling-loss safety interlock.
static bool     s_coolingFault = false;      // latched until acknowledged
static bool     s_faultTiming = false;       // grace window is running
static uint32_t s_faultAtMs = 0;

// Momentary power-button pulses (PC channels).
static uint8_t  s_pulseReq[PSU_COUNT]          = { 0 };      // 0 none, 1 short, 2 long (force-off)
static bool     s_pulsing[PSU_COUNT]           = { false };
static uint32_t s_pulseEndMs[PSU_COUNT]        = { 0 };
static uint32_t s_pulseCooldownAtMs[PSU_COUNT] = { 0 };

// A PC commanded on while cooling isn't running yet — waits for cooling, then
// gets pressed by POWER_tick. Counts as "on" so cooling stays up meanwhile.
static bool     s_startPending[PSU_COUNT]      = { false };
static uint32_t s_startPendingAtMs[PSU_COUNT]  = { 0 };

// A PC is "active" (on / making heat) if it's actually running OR waiting to
// start. This is the source of truth for a PC — never a lingering intent — so
// an external shutdown or a mis-wired monitor is reflected immediately.
static bool pcActive(uint8_t pc)
{
    return s_startPending[pc] || POWER_isRunning(pc);
}

static void driveStart(uint8_t psu, bool on)
{
    digitalWrite(START_PIN[psu], (on == START_ACTIVE_HIGH) ? HIGH : LOW);
}

// Queue a momentary button press on a pulse channel — ignored if one is already
// pending/active or during the brief cooldown after the last press (so a double
// tap or monitor lag can't accidentally toggle the PC back).
static void requestPress(uint8_t psu)
{
    if (s_pulseReq[psu] == 0 && !s_pulsing[psu] &&
        (int32_t)(millis() - s_pulseCooldownAtMs[psu]) >= 0)
        s_pulseReq[psu] = 1;
}

// Queue a long hold (ATX hard power-off). Always allowed; overrides a short press.
static void requestForceOff(uint8_t psu)
{
    s_pulseReq[psu] = 2;
}

// Non-blocking pulse engine: assert the PWR_SW for the requested duration, then
// release. Call every POWER_tick().
static void tickPulses()
{
    for (uint8_t p = 0; p < PSU_COUNT; p++)
    {
        if (!CH_IS_PULSE[p])
            continue;

        if (s_pulsing[p])
        {
            if ((int32_t)(millis() - s_pulseEndMs[p]) >= 0)
            {
                driveStart(p, false);   // release the button
                s_pulsing[p] = false;
                s_pulseCooldownAtMs[p] = millis() + PWR_PULSE_COOLDOWN_MS;
            }
        }
        else if (s_pulseReq[p] != 0)
        {
            uint32_t dur = (s_pulseReq[p] == 2) ? PWR_PULSE_LONG_MS : PWR_PULSE_SHORT_MS;
            driveStart(p, true);        // press the button
            s_pulseEndMs[p] = millis() + dur;
            s_pulsing[p] = true;
            s_pulseReq[p] = 0;
        }
    }
}

void POWER_init()
{
    for (uint8_t p = 0; p < PSU_COUNT; p++)
    {
        s_commanded[p] = false;
        pinMode(START_PIN[p], OUTPUT);
        driveStart(p, false);            // everything off / released at boot
        pinMode(MONITOR_PIN[p], MONITOR_MODE);

        s_pulseReq[p] = 0;
        s_pulsing[p] = false;
        s_pulseCooldownAtMs[p] = 0;
        s_startPending[p] = false;
    }

    s_coolingAuto = false;
    s_coolingOffPending = false;
    s_coolingFault = false;
    s_faultTiming = false;
}

bool POWER_anyPcOn()
{
    // "On" = drawing power / making heat: actually running OR waiting to start.
    // Monitor-based (via pcActive), so an external shutdown drops the lock and a
    // mis-wired monitor clearing releases it, both immediately.
    for (uint8_t p = PSU_PC1; p <= PSU_PC3; p++)
        if (pcActive(p))
            return true;
    return false;
}

bool POWER_isCoolingLocked()
{
    return POWER_anyPcOn();
}

void POWER_setCommanded(uint8_t psu, bool on)
{
    if (psu >= PSU_COUNT)
        return;

    // --- Cooling supply (level control) -----------------------------------
    if (psu == PSU_COOLING)
    {
        // Can't switch cooling off while any PC is active.
        if (!on && POWER_anyPcOn())
            return;

        s_commanded[PSU_COOLING] = on;
        driveStart(PSU_COOLING, on);
        s_coolingAuto = false;          // direct control — manual cooling never auto-offs
        s_coolingOffPending = false;
        return;
    }

    // --- A PC (momentary power button) ------------------------------------
    if (on)
    {
        if (pcActive(psu))
            return;                     // already on / coming on — nothing to do

        // Ensure cooling is on so this PC can spin up. Only mark it auto-driven
        // if *this* start is what turned cooling on — cooling the user started
        // separately stays manual and won't auto-off after the PCs stop.
        if (!s_commanded[PSU_COOLING])
        {
            s_commanded[PSU_COOLING] = true;
            driveStart(PSU_COOLING, true);
            s_coolingAuto = true;
        }
        s_coolingOffPending = false;

        if (POWER_isRunning(PSU_COOLING))
        {
            requestPress(psu);          // cooling ready — press to start now
            s_startPending[psu] = false;
        }
        else
        {
            // Defer: POWER_tick presses it once cooling is actually running.
            s_startPending[psu] = true;
            s_startPendingAtMs[psu] = millis() + COOLING_SPINUP_TIMEOUT_MS;
        }
    }
    else // turning a PC off
    {
        s_startPending[psu] = false;    // cancel a not-yet-started deferred start
        if (POWER_isRunning(psu))
            requestPress(psu);          // press to soft-off a running PC
    }
}

void POWER_tick()
{
    // Advance any in-flight power-button pulses (PC channels).
    tickPulses();

    // --- Deferred PC starts -----------------------------------------------
    // A PC commanded on while cooling was still spinning up: press it the moment
    // cooling is actually running. If cooling never comes up within the window,
    // abandon the start and latch a fault.
    for (uint8_t p = PSU_PC1; p <= PSU_PC3; p++)
    {
        if (!s_startPending[p])
            continue;

        if (POWER_isRunning(PSU_COOLING))
        {
            requestPress(p);
            s_startPending[p] = false;
        }
        else if ((int32_t)(millis() - s_startPendingAtMs[p]) >= 0)
        {
            s_startPending[p] = false;   // cooling never came up — give up
            s_coolingFault = true;
        }
    }

    // --- Cooling auto-off (monitor-driven) --------------------------------
    // Once no PC is active (commanded or still running) and cooling is auto-
    // driven, shut it down after the cooldown. A PC coming back cancels it.
    // Signed compares handle millis() rollover.
    if (s_commanded[PSU_COOLING] && s_coolingAuto && !POWER_anyPcOn())
    {
        if (!s_coolingOffPending)
        {
            s_coolingOffPending = true;
            s_coolingOffAtMs = millis() + COOLING_OFF_DELAY_MS;
        }
        else if ((int32_t)(millis() - s_coolingOffAtMs) >= 0)
        {
            s_commanded[PSU_COOLING] = false;
            driveStart(PSU_COOLING, false);
            s_coolingAuto = false;
            s_coolingOffPending = false;
        }
    }
    else
    {
        s_coolingOffPending = false;   // a PC came back, or cooling is manual — cancel
    }

    // --- Cooling-loss interlock -------------------------------------------
    // Any PC actually running (per its monitor) while the cooling supply is not
    // actually running. First try the easy recovery — make sure cooling is
    // commanded on. If it still isn't running after the grace window, the pump/
    // PSU has really failed: force every running PC off and latch a fault.
    bool pcRunning = POWER_isRunning(PSU_PC1) || POWER_isRunning(PSU_PC2) || POWER_isRunning(PSU_PC3);

    if (pcRunning && !POWER_isRunning(PSU_COOLING))
    {
        if (!s_commanded[PSU_COOLING])
        {
            s_commanded[PSU_COOLING] = true;
            driveStart(PSU_COOLING, true);
            s_coolingAuto = true;
        }

        if (!s_faultTiming)
        {
            s_faultTiming = true;
            s_faultAtMs = millis() + COOLING_FAULT_GRACE_MS;
        }
        else if ((int32_t)(millis() - s_faultAtMs) >= 0)
        {
            for (uint8_t p = PSU_PC1; p <= PSU_PC3; p++)
            {
                s_startPending[p] = false;   // abandon any deferred start
                if (POWER_isRunning(p))
                    requestForceOff(p);      // long PWR_SW hold = ATX hard power-off
            }
            s_coolingFault = true;
            s_faultTiming = false;
        }
    }
    else
    {
        s_faultTiming = false;   // condition cleared (or no PC running) — reset grace
    }
}

void POWER_toggle(uint8_t psu)
{
    if (psu < PSU_COUNT)
        POWER_setCommanded(psu, !POWER_isCommanded(psu));
}

bool POWER_isCommanded(uint8_t psu)
{
    if (psu >= PSU_COUNT)
        return false;
    // A PC's "on" is its real state — running or waiting for cooling to start it
    // (so the button reads STOP the whole time, and an external shutdown flips it
    // back to START). Cooling keeps a held command level.
    if (CH_IS_PULSE[psu])
        return pcActive(psu);
    return s_commanded[psu];
}

bool POWER_isRunning(uint8_t psu)
{
    if (psu >= PSU_COUNT)
        return false;

    int level = digitalRead(MONITOR_PIN[psu]);
    return RUNNING_ACTIVE_HIGH ? (level == HIGH) : (level == LOW);
}

bool POWER_isCoolingFault() { return s_coolingFault; }

void POWER_ackCoolingFault() { s_coolingFault = false; }
