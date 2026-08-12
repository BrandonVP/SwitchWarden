/*
 ===========================================================================
 Name        : SwitchWarden.ino
 Author      : Brandon Van Pelt
 Description : Front panel for a 3-computer + water-cooling power controller,
               on a Teensy 4.0 with a 480x320 ILI9488 LCD and FT6206 touch
               (teensy40_480x320_lcd board).

               Tabs: Power (start/stop the 4 PSUs), Monitor (placeholder),
               Settings (theme picker). The cooling PSU is forced on and locked
               whenever any PC is running — see PowerControl.

               Built on the EmbeddedGFX library. Display/touch wiring matches
               the demo (CS=10, DC=9, MOSI=11, MISO=12, SCK=13, BL=14, RST=15;
               FT6206 on I2C SDA0=18 / SCL0=19). GPIO/PSU pins live in
               PowerControl.cpp.
 ===========================================================================
 */

#include <SPI.h>
#include <ILI9488_t3.h>
#include <Adafruit_FT6206.h>
#include <TimeLib.h>            // status-bar clock (Teensy RTC)

#include "appConfig.h"          // GFX overrides + <EmbeddedGFX.h> + project enums
#include <apps/ThemeApp.h>      // library theme picker
#include "ILI9488Adapter.h"
#include "FT6206Adapter.h"
#include "PowerControl.h"
#include "PowerApp.h"
#include "MonitorApp.h"
#include "font_Michroma.h"

// --- Pins (from teensy40_480x320_lcd schematic) ----------------------------
#define TFT_CS 10
#define TFT_DC  9
#define LCD_BL 14

// --- Hardware drivers ------------------------------------------------------
ILI9488_t3      tft = ILI9488_t3(&SPI, TFT_CS, TFT_DC);
Adafruit_FT6206 ts  = Adafruit_FT6206();

// --- EmbeddedGFX adapters + registry ---------------------------------------
ILI9488Adapter gfxDisplay(tft);
FT6206Adapter  gfxTouch(ts);
App app;

DMAMEM UserInterfaceClass appButtons[GFX_APP_BUTTON_SIZE];
UserInterfaceClass        menuButtons[GFX_MENU_BUTTON_SIZE];

// --- Status bar (clock left; no battery on this build) ---------------------
#define STATUS_CLOCK_W 78

time_t getTeensy3Time() { return Teensy3Clock.get(); }

void drawClock()
{
    char t[16], d[16];
    snprintf(t, sizeof(t), "%02d:%02d:%02d", hour(), minute(), second());
    snprintf(d, sizeof(d), "%02d/%02d/%02d", month(), day(), year() % 100);

    tft.setTextColor(gfxTheme.btnTextColor);
    tft.setFont(Michroma_8);
    tft.fillRect(2, 6, STATUS_CLOCK_W, 32, gfxTheme.menuBg);
    tft.drawString(t, strlen(t), 4, 8);
    tft.drawString(d, strlen(d), 4, 24);
    tft.setFont(Michroma_11);
}

void updateStatusBar()
{
    static uint32_t last = 0;
    if (millis() - last < 1000) return;
    last = millis();

    tft.useFrameBuffer(false);
    drawClock();
    tft.useFrameBuffer(true);
    drawClock();
}

// --- Menu bar --------------------------------------------------------------
void createMenuBtns()
{
    menuButtons[0].setButton( 82, 0, 208, 45, APP_POWER,   true, 0, "Power",    ALIGN_CENTER, gfxTheme.menuBg, gfxTheme.menuBg, gfxTheme.btnTextColor);
    menuButtons[1].setButton(213, 0, 339, 45, APP_MONITOR, true, 0, "Monitor",  ALIGN_CENTER, gfxTheme.menuBg, gfxTheme.menuBg, gfxTheme.btnTextColor);
    menuButtons[2].setButton(344, 0, 470, 45, APP_SETTINGS_MENU, true, 0, "Settings", ALIGN_CENTER, gfxTheme.menuBg, gfxTheme.menuBg, gfxTheme.btnTextColor);
}

// Draw the top menu bar. Also used as ThemeApp's menu-redraw hook.
void drawMenuBar()
{
    GUI_I.drawSquareBtn(0,  0, GFX_SCREEN_WIDTH, 45, "", gfxTheme.menuBg, gfxTheme.menuBg, gfxTheme.menuBg, ALIGN_CENTER);
    GUI_I.drawSquareBtn(0, 45, GFX_SCREEN_WIDTH, GFX_MENU_BAR_HEIGHT, "", gfxTheme.menuBorder, gfxTheme.menuBorder, gfxTheme.menuBorder, ALIGN_CENTER);

    createMenuBtns();
    uint8_t state = 0;
    while (GUI_I.drawPage(menuButtons, state, GFX_MENU_BUTTON_SIZE));
    GUI_I.setGraphicLoaderState(0);

    // Underline the active app's tab (matches the on-tap highlight). Menu ids
    // line up with menuButtons order (power=0, monitor=1, settings=2).
    gfx_menu_id_t activeMenu = app.getActiveMenu();
    if (activeMenu < GFX_MENU_BUTTON_SIZE)
    {
        UserInterfaceClass& mb = menuButtons[activeMenu];
        GUI_I.drawSquareBtn(mb.getXStart(), 45, mb.getXStop(), 50, "", gfxTheme.btnColor, gfxTheme.btnColor, mb.getBorderColor(), ALIGN_CENTER);
    }

    drawClock();
    tft.updateScreen();
}

// --- App registration ------------------------------------------------------
void registerApps()
{
    // Power and Monitor open directly. Settings opens a menu-landing (generic
    // library behaviour) that lists the settings apps assigned to MENU_settings.
    app.add(MENU_power,    "Power",    APP_POWER,         power_handler,    power_createBtns);
    app.add(MENU_monitor,  "Monitor",  APP_MONITOR,       monitor_handler,  monitor_createBtns);
    app.add(MENU_settings, "Settings", APP_SETTINGS_MENU, GFX_menuInput,    GFX_createMenu);
    app.add(MENU_settings, "Themes",   APP_THEME,         ThemeApp_handler, ThemeApp_createBtns);
}

// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    setSyncProvider(getTeensy3Time);   // status-bar clock from the Teensy RTC

    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

    ts.begin(40);
    tft.begin();
    tft.useFrameBuffer(true);
    tft.fillScreen(ILI9488_BLACK);
    tft.setRotation(1);
    tft.setFont(Michroma_11);

    GUI_I.begin(gfxDisplay, gfxTouch, appButtons, menuButtons);
    GUI_I.setApp(&app);

    // Themes: RAM-only (no persistence). Repaint the menu bar on palette change.
    ThemeApp_setMenuRedraw(drawMenuBar);
    ThemeApp_begin();

    POWER_init();       // GPIOs off; safe state at boot

    registerApps();
    app.init();         // first registered app (Power) is shown on load

    drawMenuBar();
}

// ---------------------------------------------------------------------------
void loop()
{
    GUI_I.buttonMonitor(menuButtons, GFX_MENU_BUTTON_SIZE);
    GUI_I.updateTouch();
    app.run();
    POWER_tick();       // cooling auto-off timer (runs on every tab)
    updateStatusBar();
}
