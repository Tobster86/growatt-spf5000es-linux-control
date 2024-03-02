
#ifndef BATTERY_WIDGET_H
#define BATTERY_WIDGET_H

#include <SDL2/SDL.h>
#include "Widget.h"
#include "system_defs.h"

#define BATT_CAPACITY_100WH     100.00f

struct sdfBatteryWidget
{
    struct SystemStatus* pStatus;
};

void BatteryWidget_Initialise(struct sdfWidget* psdcWidget,
                              float fltXOffset,
                              float fltYOffset,
                              float fltWidth,
                              float fltHeight,
                              struct SystemStatus* pStatus);

void BatteryWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer);

void BatteryWidget_ScreenChanged(struct sdfWidget* psdcWidget);

#endif

