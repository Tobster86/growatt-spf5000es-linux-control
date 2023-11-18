
#ifndef WIDGET_H
#define WIDGET_C

#include <SDL2/SDL.h>
#include "GaugeWidget.h"

typedef void (*Update)(struct sdfWidget* pWidget, SDL_Renderer* pRenderer);
typedef void (*ScreenChanged)(struct sdfWidget* pWidget);

struct sdfWidget
{
    //Conceptual layout. 0.0f and 1.0f -> near and far screen edges.
    float fltXOffset;
    float fltYOffset;
    float fltWidth;
    float fltHeight;

    //Actual layout. In pixels.
    uint32_t lXPos;
    uint32_t lYPos;
    uint32_t lWidth;
    uint32_t lHeight;
    
    Update update;
    ScreenChanged screenChanged;
    
    union
    {
        struct sdfGaugeWidget gaugeWidget;
    };
};

void Widget_Initialise(struct sdfWidget* pWidget,
                       float fltXOffset,
                       float fltYOffset,
                       float fltWidth,
                       float fltHeight,
                       Update update,
                       ScreenChanged screenChanged);
                       
void Widget_ScreenChanged(struct sdfWidget* pWidget, int lWidth, int lHeight);

#endif