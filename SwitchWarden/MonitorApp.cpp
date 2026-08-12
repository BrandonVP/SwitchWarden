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

    b[0].setButton(40, 130, 440, 190, 0, true, 12, "Monitor - coming soon", ALIGN_CENTER,
                   gfxTheme.background, gfxTheme.background, gfxTheme.btnTextColor);
    b[0].setClickable(false);
    b[0].setTextSize(16);

    return 1;
}

void monitor_handler(int userInput)
{
    (void)userInput; // nothing interactive yet
}
