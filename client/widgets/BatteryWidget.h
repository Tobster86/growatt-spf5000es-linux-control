
#ifndef BATTERY_WIDGET_H
#define BATTERY_WIDGET_H

#include <SDL2/SDL.h>
#include "Widget.h"

struct sdfBatteryWidget
{
    uint32_t* plBatteryPercent;
};

void BatteryWidget_Initialise(struct sdfWidget* psdcWidget,
                              float fltXOffset,
                              float fltYOffset,
                              float fltWidth,
                              float fltHeight,
                              uint32_t* plBatteryPercent);

void BatteryWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer);

void BatteryWidget_ScreenChanged(struct sdfWidget* psdcWidget);

#endif

