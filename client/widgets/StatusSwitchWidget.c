
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
static SDL_Texture* textureSwitchBatt = NULL;
static SDL_Texture* textureSwitchGrid = NULL;

static TTF_Font* font = NULL;

static void LoadTexture(SDL_Renderer* pRenderer, bool* bFailed, const char* path, SDL_Texture** texture)
{
    SDL_Surface* surface = IMG_Load(path);
    
    if(NULL != surface)
    {
        *texture = SDL_CreateTextureFromSurface(pRenderer, surface);
        SDL_SetTextureBlendMode(*texture, SDL_BLENDMODE_MOD);
        SDL_FreeSurface(surface);
    }
    else if(!*bFailed)
    {
        printf("Failed to load %s.\n", path);
        *bFailed = true;
    }
}

static void StatusSwitchWidget_InitAssets(SDL_Renderer* pRenderer)
{
    static bool bFailed = false;
    
    LoadTexture(pRenderer, &bFailed, "assets/pylon.png", &texturePylon);
    LoadTexture(pRenderer, &bFailed, "assets/inverter.png", &textureInverter);
    LoadTexture(pRenderer, &bFailed, "assets/switch_batt.png", &textureSwitchBatt);
    LoadTexture(pRenderer, &bFailed, "assets/switch_grid.png", &textureSwitchGrid);
    
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
                              struct SystemStatus* pSystemStatus,
                              uint16_t* pnSwitchRequest)
{
    psdcWidget->psdcStatusSwitchWidget = malloc(sizeof(struct sdfStatusSwitchWidget));
    psdcWidget->psdcStatusSwitchWidget->pSystemStatus = pSystemStatus;
    psdcWidget->psdcStatusSwitchWidget->pnSwitchRequest = pnSwitchRequest;
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
    
    //Draw batt/grid switch.
    SDL_Rect rectSwitch = { psdcWidget->lXPos,
                            psdcWidget->lYPos + psdcWidget->lWidth,
                            psdcWidget->lWidth,
                            psdcWidget->lHeight - psdcWidget->lWidth };

    SDL_SetRenderDrawColor(pRenderer, 255, 255, 255, 255);
    SDL_RenderFillRect(pRenderer, &rectSwitch);
    
    if(SYSTEM_STATE_DAY == *psdcWidget->psdcStatusSwitchWidget->pnSwitchRequest ||
       (SYSTEM_STATE_NO_CHANGE == *psdcWidget->psdcStatusSwitchWidget->pnSwitchRequest &&
        SYSTEM_STATE_DAY == psdcWidget->psdcStatusSwitchWidget->pSystemStatus->nSystemState))
    {
        //Handle in day (battery) position.
        SDL_RenderCopy(pRenderer, textureSwitchBatt, NULL, &rectSwitch);
    }
    else if(SYSTEM_STATE_BYPASS == *psdcWidget->psdcStatusSwitchWidget->pnSwitchRequest ||
            (SYSTEM_STATE_NO_CHANGE == *psdcWidget->psdcStatusSwitchWidget->pnSwitchRequest &&
             SYSTEM_STATE_BYPASS == psdcWidget->psdcStatusSwitchWidget->pSystemStatus->nSystemState) ||
            SYSTEM_STATE_NIGHT == psdcWidget->psdcStatusSwitchWidget->pSystemStatus->nSystemState)
    {
        //Handle in night/bypass (grid) position.
        SDL_RenderCopy(pRenderer, textureSwitchGrid, NULL, &rectSwitch);
    }
}

void StatusSwitchWidget_ScreenChanged(struct sdfWidget* psdcWidget)
{
    
}

void StatusSwitchWidget_Click(struct sdfWidget* psdcWidget, int lXPos, int lYPos)
{
    if(lXPos >= psdcWidget->lXPos &&
       lXPos <= psdcWidget->lXPos + psdcWidget->lWidth &&
       lYPos >= psdcWidget->lYPos + psdcWidget->lWidth &&
       lYPos <= psdcWidget->lYPos + psdcWidget->lWidth + (psdcWidget->lHeight - psdcWidget->lWidth))
    {
        //Raise a switch to batt/grid request accordingly.
        if(SYSTEM_STATE_DAY == psdcWidget->psdcStatusSwitchWidget->pSystemStatus->nSystemState)
            *psdcWidget->psdcStatusSwitchWidget->pnSwitchRequest = SYSTEM_STATE_BYPASS;
        else if(SYSTEM_STATE_BYPASS == psdcWidget->psdcStatusSwitchWidget->pSystemStatus->nSystemState)
            *psdcWidget->psdcStatusSwitchWidget->pnSwitchRequest = SYSTEM_STATE_DAY;

        printf("User tapped the batt/grid switch.\n");
        //Note: Ignore if we're on night mode. Also ignore further user inputs until the system has responded.
    }
}

