
#ifndef GAUGE_WIDGET_H
#define GAUGE_WIDGET_H

#include <SDL2/SDL.h>
#include "Widget.h"

struct sdfGaugeWidget
{
    float fltMin;
    float fltMax;
};

void GaugeWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer);

#endif

