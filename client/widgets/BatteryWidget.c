
#include "BatteryWidget.h"
#include <SDL2/SDL_image.h>
#include <stdbool.h>

#define BATT_TOP_OFFSET     0.15f
#define BATT_HEIGHT_OFFSET  0.75f
#define BATT_WIDTH_OFFSET   0.3f

const SDL_Colour colBatt = { 100, 100, 100, 255 };
const SDL_Colour col50 = { 0, 150, 0, 255 };
const SDL_Colour col25 = { 150, 150, 0, 255 };
const SDL_Colour colLow = { 150, 0, 0, 255 };

static SDL_Texture* textureBattery = NULL;

static void BatteryWidget_InitAssets(SDL_Renderer* pRenderer)
{
    static bool bFailed = false;
    SDL_Surface* surface = IMG_Load("assets/batt.png");
    
    if(NULL != surface)
    {
        textureBattery = SDL_CreateTextureFromSurface(pRenderer, surface);
        SDL_FreeSurface(surface);
    }
    else if(!bFailed)
    {
        printf("Failed to load battery texture.\n");
        bFailed = true;
    }
}

void BatteryWidget_Initialise(struct sdfWidget* psdcWidget,
                              float fltXOffset,
                              float fltYOffset,
                              float fltWidth,
                              float fltHeight,
                              uint32_t* plBatteryPercent)
{
    psdcWidget->psdcBatteryWidget = malloc(sizeof(struct sdfBatteryWidget));
    psdcWidget->psdcBatteryWidget->plBatteryPercent = plBatteryPercent;
    Widget_Initialise(psdcWidget,
                      fltXOffset,
                      fltYOffset,
                      fltWidth,
                      fltHeight,
                      BatteryWidget_Update,
                      BatteryWidget_ScreenChanged,
                      NULL);
}

void BatteryWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer)
{
    //Initialise assets if needed.
    if(NULL == textureBattery)
    {
        BatteryWidget_InitAssets(pRenderer);
    }

    //Draw battery body.
    SDL_SetRenderDrawColor(pRenderer, colBatt.r, colBatt.g, colBatt.b, colBatt.a);
    
    SDL_Rect rectBckg = { psdcWidget->lXPos,
                          psdcWidget->lYPos,
                          psdcWidget->lWidth,
                          psdcWidget->lHeight };
                          
    SDL_RenderCopy(pRenderer, textureBattery, NULL, &rectBckg);
    
    //Draw capacity bar.
    uint32_t lAreaTopOffset = (((float)psdcWidget->lHeight) * BATT_TOP_OFFSET + 0.5f);
    uint32_t lAreaHeight = (((float)psdcWidget->lHeight * BATT_HEIGHT_OFFSET) + 0.5f);

    uint32_t lAreaWidth = (((float)psdcWidget->lWidth) * (1.0f - BATT_WIDTH_OFFSET) + 0.5f);
    uint32_t lAreaSideOffset = (psdcWidget->lWidth - lAreaWidth) / 2;
    
    if(*psdcWidget->psdcBatteryWidget->plBatteryPercent >= 50)
        SDL_SetRenderDrawColor(pRenderer, col50.r, col50.g, col50.b, col50.a);
    else if(*psdcWidget->psdcBatteryWidget->plBatteryPercent >= 25)
        SDL_SetRenderDrawColor(pRenderer, col25.r, col25.g, col25.b, col25.a);
    else
        SDL_SetRenderDrawColor(pRenderer, colLow.r, colLow.g, colLow.b, colLow.a);
        
    uint32_t lPowerHeight = (int)((((float)lAreaHeight / 100.0f) * (float)*psdcWidget->psdcBatteryWidget->plBatteryPercent) + 0.5f);
        
    SDL_Rect rectBatt = { psdcWidget->lXPos + lAreaSideOffset,
                          psdcWidget->lYPos + lAreaTopOffset + (lAreaHeight - lPowerHeight),
                          lAreaWidth,
                          lPowerHeight };
    SDL_RenderFillRect(pRenderer, &rectBatt);
}

void BatteryWidget_ScreenChanged(struct sdfWidget* psdcWidget)
{
    
}

