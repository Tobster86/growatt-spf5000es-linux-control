
#include "GaugeWidget.h"

void GaugeWidget_Initialise(struct sdfWidget* psdcWidget,
                            float fltXOffset,
                            float fltYOffset,
                            float fltWidth,
                            float fltHeight,
                            float fltMin,
                            float fltMax)
{
    psdcWidget->psdcGaugeWidget = malloc(sizeof(struct sdfGaugeWidget));
    Widget_Initialise(psdcWidget, fltXOffset, fltYOffset, fltWidth, fltHeight, GaugeWidget_Update, GaugeWidget_ScreenChanged);
}

void GaugeWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer)
{
    SDL_SetRenderDrawColor(pRenderer, 0, 0, 255, 255);
    SDL_Rect rectangle = { psdcWidget->lXPos, psdcWidget->lYPos, psdcWidget->lWidth, psdcWidget->lHeight };
    SDL_RenderFillRect(pRenderer, &rectangle);
}

void GaugeWidget_ScreenChanged(struct sdfWidget* psdcWidget)
{
    
}

