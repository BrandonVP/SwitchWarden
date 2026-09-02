/*
===========================================================================
Name        : appConfig.h
Author      : Brandon Van Pelt
Description : Project configuration for SwitchWarden — screen overrides plus
              the project's own menu and app id enums. Include this instead of
              <EmbeddedGFX.h> directly so the GFX_* overrides are seen first.
===========================================================================
*/
#ifndef APPCONFIG_H
#define APPCONFIG_H

// --- Library configuration overrides (must precede <EmbeddedGFX.h>) --------
#define GFX_SCREEN_WIDTH     480
#define GFX_SCREEN_HEIGHT    320
#define GFX_APP_BUTTON_SIZE  30
#define GFX_MENU_BUTTON_SIZE 3

#include <EmbeddedGFX.h>

// --- Top menu tabs ---------------------------------------------------------
// Ids match the on-screen order (used to underline the active tab).
enum Menus {
    MENU_power = 0,
    MENU_monitor,
    MENU_settings
};

// --- Apps ------------------------------------------------------------------
// Power and Monitor tabs open their app directly. Settings opens a menu-landing
// (a grid of settings apps) so more settings can be added alongside Themes.
enum AppLabels {
    APP_POWER = 0,       // Power   tab — 4-PSU control (opens directly)
    APP_MONITOR,         // Monitor tab — placeholder (opens directly)
    APP_SETTINGS_MENU,   // Settings tab — landing page listing settings apps
    APP_THEME,           // Settings > Themes (library theme picker)
    APP_ABOUT,           // Settings > About (version + project info)
    APP_COUNT
};

#endif // APPCONFIG_H
