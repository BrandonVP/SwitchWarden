/*
===========================================================================
Name        : PowerApp.cpp
Author      : Brandon Van Pelt
Description : Power tab UI (see PowerApp.h). Three buttons per PSU row: a name
              label, a live run-status pill, and a start/stop action button.
===========================================================================
*/
#include "PowerApp.h"
#include "PowerControl.h"

// Three buttons per PSU row.
static uint8_t nameIdx(uint8_t p)   { return (uint8_t)(3 * p + 0); }
static uint8_t statusIdx(uint8_t p) { return (uint8_t)(3 * p + 1); }
static uint8_t actionIdx(uint8_t p) { return (uint8_t)(3 * p + 2); }

// Action button click-return values (1..PSU_COUNT; -1 means "no tap").
static const int ACTION_BASE = 1;

// Fixed accent colors (RGB565) that read the same across themes.
static const uint16_t COL_RUN  = 0x07E0; // green — running
static const uint16_t COL_LOCK = 0x8410; // grey  — cooling locked on
static const uint16_t COL_DIM  = 0x8410; // grey  — "OFF" / disabled text

static const char* const PSU_NAMES[PSU_COUNT] = { "PC 1", "PC 2", "PC 3", "Cooling" };

static bool s_lastRunning[PSU_COUNT];
static bool s_lastCommanded[PSU_COUNT];

static void styleStatus(uint8_t p)
{
    UserInterfaceClass& s = GUI_I.appButtons()[statusIdx(p)];
    bool run = POWER_isRunning(p);
    s.setText(run ? "RUNNING" : "OFF");
    s.setBgColor(run ? COL_RUN : gfxTheme.background);
    s.setBorderColor(run ? COL_RUN : gfxTheme.btnBorder);
    s.setTextColor(run ? 0x0000 : COL_DIM);
}

static void styleAction(uint8_t p)
{
    UserInterfaceClass& a = GUI_I.appButtons()[actionIdx(p)];
    bool on = POWER_isCommanded(p);
    bool locked = (p == PSU_COOLING) && POWER_isCoolingLocked();

    if (locked)
    {
        a.setText("LOCKED ON");
        a.setBgColor(COL_LOCK);
        a.setBorderColor(COL_LOCK);
        a.setTextColor(COL_DIM);
        a.setClickable(false);
    }
    else
    {
        a.setText(on ? "STOP" : "START");
        a.setBgColor(on ? gfxTheme.orangeBtn : gfxTheme.btnColor);
        a.setBorderColor(on ? gfxTheme.orangeBtn : gfxTheme.btnBorder);
        a.setTextColor(on ? 0x0000 : gfxTheme.btnText);
        a.setClickable(true);
    }
}

uint8_t power_createBtns(void)
{
    UserInterfaceClass* b = GUI_I.appButtons();

    for (uint8_t p = 0; p < PSU_COUNT; p++)
    {
        int y = 60 + p * 63;

        // Name label — plain text on the app background (non-clickable).
        b[nameIdx(p)].setButton(12, y, 150, y + 54, 0, true, 8, PSU_NAMES[p], ALIGN_LEFT,
                                gfxTheme.background, gfxTheme.background, gfxTheme.btnTextColor);
        b[nameIdx(p)].setClickable(false);
        b[nameIdx(p)].setTextSize(16);

        // Run-status pill (non-clickable) — styled from the monitor input.
        b[statusIdx(p)].setButton(158, y, 300, y + 54, 0, true, 10, "OFF", ALIGN_CENTER,
                                  gfxTheme.background, gfxTheme.btnBorder, COL_DIM);
        b[statusIdx(p)].setClickable(false);
        b[statusIdx(p)].setTextSize(13);

        // Start/stop action button.
        b[actionIdx(p)].setButton(308, y, 468, y + 54, ACTION_BASE + p, true, 10, "START", ALIGN_CENTER,
                                  gfxTheme.btnColor, gfxTheme.btnBorder, gfxTheme.btnText);
        b[actionIdx(p)].setTextSize(16);

        styleStatus(p);
        styleAction(p);
        s_lastRunning[p]   = POWER_isRunning(p);
        s_lastCommanded[p] = POWER_isCommanded(p);
    }

    return (uint8_t)(PSU_COUNT * 3);
}

// Redraw one PSU's status + action buttons to the panel.
static void refreshRow(uint8_t p)
{
    styleStatus(p);
    styleAction(p);
    GUI_I.updateButton(statusIdx(p));
    GUI_I.updateButton(actionIdx(p));
}

void power_handler(int userInput)
{
    // A tap on an action button toggles that PSU. Because starting/stopping a PC
    // can lock or unlock the cooling supply, redraw every row.
    if (userInput >= ACTION_BASE && userInput < ACTION_BASE + PSU_COUNT)
    {
        POWER_toggle((uint8_t)(userInput - ACTION_BASE));

        for (uint8_t p = 0; p < PSU_COUNT; p++)
        {
            refreshRow(p);
            s_lastRunning[p]   = POWER_isRunning(p);
            s_lastCommanded[p] = POWER_isCommanded(p);
        }
        GUI_I.updateScreen();
        return;
    }

    bool dirty = false;

    // Run-status pills follow the monitor inputs.
    for (uint8_t p = 0; p < PSU_COUNT; p++)
    {
        bool run = POWER_isRunning(p);
        if (run != s_lastRunning[p])
        {
            s_lastRunning[p] = run;
            styleStatus(p);
            GUI_I.updateButton(statusIdx(p));
            dirty = true;
        }
    }

    // Commanded state can change with no tap — the cooling auto-on/auto-off
    // logic. If any changed, restyle all action buttons since the cooling lock
    // depends on whether a PC is running.
    bool cmdChanged = false;
    for (uint8_t p = 0; p < PSU_COUNT; p++)
    {
        bool cmd = POWER_isCommanded(p);
        if (cmd != s_lastCommanded[p])
        {
            s_lastCommanded[p] = cmd;
            cmdChanged = true;
        }
    }
    if (cmdChanged)
    {
        for (uint8_t p = 0; p < PSU_COUNT; p++)
        {
            styleAction(p);
            GUI_I.updateButton(actionIdx(p));
        }
        dirty = true;
    }

    if (dirty)
        GUI_I.updateScreen();
}
