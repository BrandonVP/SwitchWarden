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
    // Gradient header: deep blue at top fading to the theme's menu blue.
    GUI_I.fillGradientV(0, 0, GFX_SCREEN_WIDTH, 45, gfxShade(gfxTheme.menuBg, -35), gfxTheme.menuBg);
    // Underline strip below the bar.
    GUI_I.drawSquareBtn(0, 45, GFX_SCREEN_WIDTH, GFX_MENU_BAR_HEIGHT, "", gfxTheme.menuBorder, gfxTheme.menuBorder, gfxTheme.menuBorder, ALIGN_CENTER);

    // Set up the tab rects (used for hit-testing + the active underline). The
    // tabs are NOT drawn as filled boxes — their labels are drawn as white text
    // straight over the gradient so the gradient shows through.
    createMenuBtns();

    tft.setFont(Michroma_16);
    tft.setTextColor(gfxTheme.menuText);
    for (uint8_t i = 0; i < GFX_MENU_BUTTON_SIZE; i++)
    {
        const char* label = menuButtons[i].getBtnText();
        int len = strlen(label);
        int w = tft.strPixelLen((char*)label, (uint16_t)len);
        int cx = (menuButtons[i].getXStart() + menuButtons[i].getXStop()) / 2;
        tft.drawString(label, len, cx - w / 2, 14);
    }
    tft.setFont(Michroma_11);

    // Underline the active app's tab. Menu ids line up with menuButtons order.
    gfx_menu_id_t activeMenu = app.getActiveMenu();
    if (activeMenu < GFX_MENU_BUTTON_SIZE)
    {
        UserInterfaceClass& mb = menuButtons[activeMenu];
        GUI_I.drawSquareBtn(mb.getXStart(), 45, mb.getXStop(), 50, "", gfxTheme.btnColor, gfxTheme.btnColor, mb.getBorderColor(), ALIGN_CENTER);
    }

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

// Light "frost" look inspired by the reference smart-panel mockup: white body,
// rich blue header, navy text, blue primary + orange (stop) accents. Applied
// after ThemeApp_begin so it is the default; the Settings > Themes picker still
// overrides it.
void applyFrostPalette()
{
    gfxTheme.background   = 0xFFFF;                // white body
    gfxTheme.menuBg       = 0x2C3A;                // rich sky blue (header base)
    gfxTheme.menuBorder   = gfxShade(0x2C3A, -35); // deep blue underline strip
    gfxTheme.btnColor     = 0x2C3A;                // blue primary / active underline
    gfxTheme.btnBorder    = 0xC618;                // light grey border
    gfxTheme.btnText      = 0xFFFF;                // white text on blue buttons
    gfxTheme.btnTextColor = 0x0A4B;                // navy body/card text
    gfxTheme.menuText     = 0xFFFF;                // white tab labels over the bar
    gfxTheme.orangeBtn    = 0xFC00;                // orange (STOP)
    gfxTheme.blackBtn     = 0x0000;
    gfxTheme.frameBorder  = 0xC618;
}

// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

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
    applyFrostPalette();   // default to the light frost look

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
}
