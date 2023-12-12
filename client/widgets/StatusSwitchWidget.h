
#ifndef STATUS_SWITCH_WIDGET_H
#define STATUS_SWITCH_WIDGET_H

#include <SDL2/SDL.h>
#include "Widget.h"

struct sdfStatusSwitchWidget
{
    struct SystemStatus* pSystemStatus;
    uint16_t* pnSwitchRequest;
};

void StatusSwitchWidget_Initialise(struct sdfWidget* psdcWidget,
                                   float fltXOffset,
                                   float fltYOffset,
                                   float fltWidth,
                                   float fltHeight,
                                   struct SystemStatus* pSystemStatus,
                                   uint16_t* pnSwitchRequest);

void StatusSwitchWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer);

void StatusSwitchWidget_ScreenChanged(struct sdfWidget* psdcWidget);

void StatusSwitchWidget_Click(struct sdfWidget* psdcWidget, int lXPos, int lYPos);

#endif

