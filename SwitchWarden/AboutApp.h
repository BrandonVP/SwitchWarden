/*
===========================================================================
Name        : AboutApp.h
Author      : Brandon Van Pelt
Description : Settings > About — firmware version + basic project info.
===========================================================================
*/
#ifndef ABOUTAPP_H
#define ABOUTAPP_H

#include "appConfig.h"

uint8_t about_createBtns(void);
void    about_handler(int userInput);

#endif // ABOUTAPP_H
