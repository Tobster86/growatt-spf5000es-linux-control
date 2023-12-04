
#ifndef GAUGE_WIDGET_H
#define GAUGE_WIDGET_H

#include <SDL2/SDL.h>
#include "Widget.h"

struct sdfGaugeWidget
{
    float fltMin;
    float fltMax;
    float fltMajInc;
    float fltMinInc;
    float fltWarn;
    float fltMinAng;
    float fltMaxAng;
    uint32_t* plValue;
};

void GaugeWidget_Initialise(struct sdfWidget* psdcWidget,
                            float fltXOffset,
                            float fltYOffset,
                            float fltWidth,
                            float fltHeight,
                            float fltMin,
                            float fltMax,
                            float fltMinInc,
                            float fltMajInc,
                            float fltWarn,
                            float fltMinAng,
                            float fltMaxAng,
                            uint32_t* plValue);

void GaugeWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer);

void GaugeWidget_ScreenChanged(struct sdfWidget* psdcWidget);

#endif

