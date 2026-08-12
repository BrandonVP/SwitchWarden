/*
===========================================================================
Name        : PowerControl.h
Author      : Brandon Van Pelt
Description : Hardware layer for the four PSUs — three PC supplies plus the
              shared water-cooling supply. Owns the start outputs, the run
              monitor inputs, and the safety rule that the cooling supply is
              forced on and locked while any PC is running.

              The front end talks only to this interface; wiring specifics live
              in PowerControl.cpp.
===========================================================================
*/
#ifndef POWERCONTROL_H
#define POWERCONTROL_H

#include <Arduino.h>

enum PsuId {
    PSU_PC1 = 0,
    PSU_PC2,
    PSU_PC3,
    PSU_COOLING,
    PSU_COUNT
};

// Cooldown before the cooling supply auto-shuts-off after the last PC stops.
#define COOLING_OFF_DELAY_MS 10000UL

// Configure GPIO directions/levels. Call once from setup().
void POWER_init();

// Advances the cooling auto-off timer. Call every loop() (independent of which
// tab is showing) so cooling shuts down COOLING_OFF_DELAY_MS after the last PC.
void POWER_tick();

// Command a supply on/off. The cooling supply cannot be turned off while any
// PC is commanded on (the request is ignored); starting any PC forces cooling
// on.
void POWER_setCommanded(uint8_t psu, bool on);

// Flip the commanded state (subject to the cooling lock).
void POWER_toggle(uint8_t psu);

// Our commanded state for a supply (what we are driving the start line to).
bool POWER_isCommanded(uint8_t psu);

// Live run status from the monitor input (independent of the command).
bool POWER_isRunning(uint8_t psu);

// True while any PC supply is commanded on — cooling is locked on in this case.
bool POWER_anyPcOn();

// True when the cooling supply's control is locked (forced on by a running PC).
bool POWER_isCoolingLocked();

#endif // POWERCONTROL_H
