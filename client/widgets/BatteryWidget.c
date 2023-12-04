
#include "BatteryWidget.h"

#define NUB_OFFSET      0.1f
#define NUB_WIDTHFRACT  3.0f
#define AREA_OFFSET_PX  10

const SDL_Colour colBatt = { 100, 100, 100, 255 };
const SDL_Colour col50 = { 0, 100, 0, 255 };
const SDL_Colour col25 = { 100, 100, 0, 255 };
const SDL_Colour colLow = { 100, 0, 0, 255 };

void BatteryWidget_Initialise(struct sdfWidget* psdcWidget,
                              float fltXOffset,
                              float fltYOffset,
                              float fltWidth,
                              float fltHeight,
                              uint32_t* plBatteryPercent)
{
    psdcWidget->psdcBatteryWidget = malloc(sizeof(struct sdfBatteryWidget));
    psdcWidget->psdcBatteryWidget->plBatteryPercent = plBatteryPercent;
    Widget_Initialise(psdcWidget, fltXOffset, fltYOffset, fltWidth, fltHeight, BatteryWidget_Update, BatteryWidget_ScreenChanged);
        
}

void BatteryWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer)
{
    //Draw battery body.
    SDL_SetRenderDrawColor(pRenderer, colBatt.r, colBatt.g, colBatt.b, colBatt.a);
    SDL_Rect rectBody = { psdcWidget->lXPos,
                          psdcWidget->lYPos + (((float)psdcWidget->lHeight) * NUB_OFFSET),
                          psdcWidget->lWidth,
                          (((float)psdcWidget->lHeight) * (1.0f - NUB_OFFSET)) };
    SDL_RenderFillRect(pRenderer, &rectBody);
    
    //Draw battery nub.
    SDL_Rect rectNub = { psdcWidget->lXPos + (((float)psdcWidget->lWidth) / NUB_WIDTHFRACT),
                         psdcWidget->lYPos,
                         (((float)psdcWidget->lWidth) / NUB_WIDTHFRACT),
                         (((float)psdcWidget->lHeight) * NUB_OFFSET) };
    SDL_RenderFillRect(pRenderer, &rectNub);
    
    //Draw capacity area.
    uint32_t lAreaHeight = (((float)psdcWidget->lHeight) * (1.0f - NUB_OFFSET)) - 2*AREA_OFFSET_PX;
    SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 255);
    SDL_Rect rectArea = { psdcWidget->lXPos + AREA_OFFSET_PX,
                          psdcWidget->lYPos + (((float)psdcWidget->lHeight) * NUB_OFFSET) + AREA_OFFSET_PX,
                          psdcWidget->lWidth - 2*AREA_OFFSET_PX,
                          lAreaHeight };
    SDL_RenderFillRect(pRenderer, &rectArea);
    
    /*printf("Address of Widget: %p\n", (void*)psdcWidget);
    printf("Address of Battery Widget: %p\n", (void*)psdcWidget->psdcBatteryWidget);
    printf("The battery percent is %d", *psdcWidget->psdcBatteryWidget->plBatteryPercent);*/
    
    //Draw capacity bar.
    if(*psdcWidget->psdcBatteryWidget->plBatteryPercent >= 50)
        SDL_SetRenderDrawColor(pRenderer, col50.r, col50.g, col50.b, col50.a);
    else if(*psdcWidget->psdcBatteryWidget->plBatteryPercent >= 25)
        SDL_SetRenderDrawColor(pRenderer, col25.r, col25.g, col25.b, col25.a);
    else
        SDL_SetRenderDrawColor(pRenderer, colLow.r, colLow.g, colLow.b, colLow.a);
        
    uint32_t lPowerHeight = (int)((((float)lAreaHeight / 100.0f) * (float)*psdcWidget->psdcBatteryWidget->plBatteryPercent) + 0.5f);
        
    SDL_Rect rectBatt = { psdcWidget->lXPos + AREA_OFFSET_PX,
                          psdcWidget->lYPos + (((float)psdcWidget->lHeight) * NUB_OFFSET) + AREA_OFFSET_PX + (lAreaHeight - lPowerHeight),
                          psdcWidget->lWidth - 2*AREA_OFFSET_PX,
                          lPowerHeight };
    SDL_RenderFillRect(pRenderer, &rectBatt);
}

void BatteryWidget_ScreenChanged(struct sdfWidget* psdcWidget)
{
    
}

