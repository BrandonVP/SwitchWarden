/*
===========================================================================
Name        : MonitorApp.cpp
Author      : Brandon Van Pelt
Description : Monitor tab placeholder (see MonitorApp.h). A single centered
              caption until real monitoring content is added.
===========================================================================
*/
#include "MonitorApp.h"

uint8_t monitor_createBtns(void)
{
    UserInterfaceClass* b = GUI_I.appButtons();

    // Match the Power tab's airy backdrop.
    GUI_I.fillGradientV(0, GFX_MENU_BAR_HEIGHT, GFX_SCREEN_WIDTH, GFX_SCREEN_HEIGHT - GFX_MENU_BAR_HEIGHT,
                        gfxShade(gfxTheme.menuBg, 85), 0xFFFF);

    b[0].setButton(40, 210, 440, 270, 0, true, 12, "Monitor - coming soon", ALIGN_CENTER,
                   0xFFFF, 0xFFFF, gfxTheme.btnTextColor);
    b[0].setClickable(false);
    b[0].setTextSize(16);

    return 1;
}

void monitor_handler(int userInput)
{
    (void)userInput; // nothing interactive yet
}
