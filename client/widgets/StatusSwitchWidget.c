
#include "StatusSwitchWidget.h"
#include "system_defs.h"
#include "spf5000es_defs.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

const SDL_Colour colGood = { 0, 255, 0, 255 };
const SDL_Colour colBad = { 255, 0, 0, 255 };

static SDL_Texture* texturePylon = NULL;
static SDL_Texture* textureInverter = NULL;

static TTF_Font* font = NULL;

static void StatusSwitchWidget_InitAssets(SDL_Renderer* pRenderer)
{
    static bool bFailed = false;
    
    SDL_Surface* surfacePylon = IMG_Load("assets/pylon.png");
    
    if(NULL != surfacePylon)
    {
        texturePylon = SDL_CreateTextureFromSurface(pRenderer, surfacePylon);
        SDL_SetTextureBlendMode(texturePylon, SDL_BLENDMODE_MOD);
        SDL_FreeSurface(surfacePylon);
    }
    else if(!bFailed)
    {
        printf("Failed to load pylon texture.\n");
        bFailed = true;
    }
    
    SDL_Surface* surfaceInverter = IMG_Load("assets/inverter.png");
    
    if(NULL != surfaceInverter)
    {
        textureInverter = SDL_CreateTextureFromSurface(pRenderer, surfaceInverter);
        SDL_SetTextureBlendMode(textureInverter, SDL_BLENDMODE_MOD);
        SDL_FreeSurface(surfaceInverter);
    }
    else if(!bFailed)
    {
        printf("Failed to load inverter texture.\n");
        bFailed = true;
    }
    
    font = TTF_OpenFont("assets/FiraCode-Regular.ttf", 120);

    if(NULL == font)
    {
        printf("Failed to load font.\n");
        bFailed = true;
    }
}

void StatusSwitchWidget_Initialise(struct sdfWidget* psdcWidget,
                              float fltXOffset,
                              float fltYOffset,
                              float fltWidth,
                              float fltHeight,
                              struct SystemStatus* pSystemStatus)
{
    psdcWidget->psdcStatusSwitchWidget = malloc(sizeof(struct sdfStatusSwitchWidget));
    psdcWidget->psdcStatusSwitchWidget->pSystemStatus = pSystemStatus;
    Widget_Initialise(psdcWidget,
                      fltXOffset,
                      fltYOffset,
                      fltWidth,
                      fltHeight,
                      StatusSwitchWidget_Update,
                      StatusSwitchWidget_ScreenChanged,
                      StatusSwitchWidget_Click);
}

void StatusSwitchWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer)
{
    //Initialise assets if needed.
    if(NULL == texturePylon)
    {
        StatusSwitchWidget_InitAssets(pRenderer);
    }

    SDL_SetRenderDrawColor(pRenderer, 0, 0, 50, 255);
    
    SDL_Rect rectBckg = { psdcWidget->lXPos,
                          psdcWidget->lYPos,
                          psdcWidget->lWidth,
                          psdcWidget->lHeight };

    SDL_RenderFillRect(pRenderer, &rectBckg);
    
    //Draw current inverter status.
    SDL_Rect rectStatus = { psdcWidget->lXPos,
                            psdcWidget->lYPos,
                            psdcWidget->lWidth,
                            psdcWidget->lWidth };
    
    switch(psdcWidget->psdcStatusSwitchWidget->pSystemStatus->nInverterState)
    {
        case DISCHARGE:
        {
            //Render battery inverter symbol.
            SDL_SetRenderDrawColor(pRenderer, colGood.r, colGood.g, colGood.b, colGood.a);
            SDL_RenderFillRect(pRenderer, &rectStatus);
            SDL_RenderCopy(pRenderer, textureInverter, NULL, &rectStatus);
        }
        break;
        
        case BYPASS:
        case AC_CHG_BYP:
        {
            if(SYSTEM_STATE_NIGHT == psdcWidget->psdcStatusSwitchWidget->pSystemStatus->nSystemState)
            {
                //Cheap electricity. Render green pylon.
                SDL_SetRenderDrawColor(pRenderer, colGood.r, colGood.g, colGood.b, colGood.a);
                SDL_RenderFillRect(pRenderer, &rectStatus);
                SDL_RenderCopy(pRenderer, texturePylon, NULL, &rectStatus);
            }
            else
            {
                //Expensive electricity. Render red pylon.
                SDL_SetRenderDrawColor(pRenderer, colBad.r, colBad.g, colBad.b, colBad.a);
                SDL_RenderFillRect(pRenderer, &rectStatus);
                SDL_RenderCopy(pRenderer, texturePylon, NULL, &rectStatus);
            }
        }
        break;
        
        default:
        {
            /* One of:
            STANDBY
            NO_USE,
            FAULT,
            FLASH,
            PV_CHG,
            AC_CHG,
            COM_CHG,
            COM_CHG_BYP,
            PV_CHG_BYP,
            PV_CHG_DISCHG,
            So just render the number for debugging purposes */
            
            if(NULL != font)
            {
                char text[4];
                sprintf(text, "%d?", psdcWidget->psdcStatusSwitchWidget->pSystemStatus->nInverterState);
            
                SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, colBad);
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(pRenderer, textSurface);
                SDL_RenderCopy(pRenderer, textTexture, NULL, &rectStatus);
                SDL_FreeSurface(textSurface);
                SDL_DestroyTexture(textTexture);
            }
        }
    }
    

    
    

    /*//Draw battery body.
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
    
    if(*psdcWidget->psdcStatusSwitch->plBatteryPercent >= 50)
        SDL_SetRenderDrawColor(pRenderer, col50.r, col50.g, col50.b, col50.a);
    else if(*psdcWidget->psdcStatusSwitch->plBatteryPercent >= 25)
        SDL_SetRenderDrawColor(pRenderer, col25.r, col25.g, col25.b, col25.a);
    else
        SDL_SetRenderDrawColor(pRenderer, colLow.r, colLow.g, colLow.b, colLow.a);
        
    uint32_t lPowerHeight = (int)((((float)lAreaHeight / 100.0f) * (float)*psdcWidget->psdcStatusSwitch->plBatteryPercent) + 0.5f);
        
    SDL_Rect rectBatt = { psdcWidget->lXPos + lAreaSideOffset,
                          psdcWidget->lYPos + lAreaTopOffset + (lAreaHeight - lPowerHeight),
                          lAreaWidth,
                          lPowerHeight };
    SDL_RenderFillRect(pRenderer, &rectBatt);*/
}

void StatusSwitchWidget_ScreenChanged(struct sdfWidget* psdcWidget)
{
    
}

void StatusSwitchWidget_Click(struct sdfWidget* psdcWidget, int lXPos, int lYPos)
{
}

