
#ifndef WIDGET_H
#define WIDGET_H

#include <SDL2/SDL.h>

struct sdfWidget;
struct sdfBatteryWidget;
struct sdfGaugeWidget;
struct sdfStatusSwitchWidget;

typedef void (*Update)(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer);
typedef void (*ScreenChanged)(struct sdfWidget* psdcWidget);
typedef void (*Click)(struct sdfWidget* psdcWidget, int lXPos, int lYPos);

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
    Click click;
    
    union
    {
        struct sdfBatteryWidget*      psdcBatteryWidget;
        struct sdfGaugeWidget*        psdcGaugeWidget;
        struct sdfStatusSwitchWidget* psdcStatusSwitchWidget;
    };
};

void Widget_Initialise(struct sdfWidget* psdcWidget,
                       float fltXOffset,
                       float fltYOffset,
                       float fltWidth,
                       float fltHeight,
                       Update update,
                       ScreenChanged screenChanged,
                       Click click);

void Widget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer);

void Widget_ScreenChanged(struct sdfWidget* psdcWidget, int lWidth, int lHeight);

void Widget_Click(struct sdfWidget* psdcWidget, int lXPos, int lYPos);

#endif
