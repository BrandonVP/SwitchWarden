/*
===========================================================================
Name        : AboutApp.cpp
Author      : Brandon Van Pelt
Description : Settings > About page (see AboutApp.h). Static info card with the
              firmware version, a one-line description, platform, and build date.
===========================================================================
*/
#include "AboutApp.h"
#include "Version.h"

uint8_t about_createBtns(void)
{
    UserInterfaceClass* b = GUI_I.appButtons();

    // Frost backdrop + floating card, matching the Power/Monitor pages.
    GUI_I.fillGradientV(0, GFX_MENU_BAR_HEIGHT, GFX_SCREEN_WIDTH, GFX_SCREEN_HEIGHT - GFX_MENU_BAR_HEIGHT,
                        gfxShade(gfxTheme.menuBg, 85), 0xFFFF);
    GUI_I.drawCard(30, 62, 420, 244, 14, 0xFFFF, 0xBDD7, 3);

    b[0].setButton(40, 74, 440, 120, 0, true, 8, "SwitchWarden", ALIGN_CENTER,
                   0xFFFF, 0xFFFF, gfxTheme.btnTextColor);
    b[0].setTextSize(24); b[0].setClickable(false);

    b[1].setButton(40, 126, 440, 156, 0, true, 8, "Version " SWITCHWARDEN_VERSION, ALIGN_CENTER,
                   0xFFFF, 0xFFFF, gfxTheme.btnColor);
    b[1].setTextSize(16); b[1].setClickable(false);

    b[2].setButton(40, 168, 440, 192, 0, true, 8, "3-PC + water-cooling controller", ALIGN_CENTER,
                   0xFFFF, 0xFFFF, gfxTheme.btnTextColor);
    b[2].setTextSize(12); b[2].setClickable(false);

    b[3].setButton(40, 198, 440, 222, 0, true, 8, "Teensy 4.0  -  EmbeddedGFX", ALIGN_CENTER,
                   0xFFFF, 0xFFFF, gfxTheme.btnTextColor);
    b[3].setTextSize(12); b[3].setClickable(false);

    b[4].setButton(40, 252, 440, 288, 0, true, 8, "Built " __DATE__, ALIGN_CENTER,
                   0xFFFF, 0xFFFF, gfxTheme.btnBorder);
    b[4].setTextSize(12); b[4].setClickable(false);

    return 5;
}

void about_handler(int userInput)
{
    (void)userInput;   // static page
}
