/*
===========================================================================
Name        : PowerApp.h
Author      : Brandon Van Pelt
Description : Power tab — one row per PSU (name, live run status, start/stop).
              Cooling is shown locked while any PC runs.
===========================================================================
*/
#ifndef POWERAPP_H
#define POWERAPP_H

#include "appConfig.h"

uint8_t power_createBtns(void);
void    power_handler(int userInput);

#endif // POWERAPP_H
