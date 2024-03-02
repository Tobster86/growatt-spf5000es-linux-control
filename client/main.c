
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "UIUtils.h"
#include "system_defs.h"
#include "comms_defs.h"
#include "tcpclient.h"
#include "Widget.h"
#include "BatteryWidget.h"
#include "GaugeWidget.h"
#include "StatusSwitchWidget.h"

#define UI_INPUT_FILTER         5000    /* Disregard all user interaction for the first 5 seconds. */

/* System status and variables. */
struct SystemStatus status;

/* UI Stuff. */
struct sdfWidget sdcWidgetBattery;
struct sdfWidget sdcWidgetPowerGauge;
struct sdfWidget sdcWidgetStatusSwitch;

struct sdfWidget* Widgets[] =
{
    &sdcWidgetBattery,
    &sdcWidgetPowerGauge,
    &sdcWidgetStatusSwitch
};

int widgetCount;
bool bRender = true;

int lXWindowSize;
int lYWindowSize;

const SDL_Colour colText = { 255, 0, 0, 255 };
/* System logic */
pthread_t processingThread;
bool quit = false;
uint16_t nSwitchRequest = SYSTEM_STATE_NO_CHANGE;

void *process(void *arg)
{
    while(!quit)
    {
        if(tcpclient_GetConnected())
        {
            tcpclient_SendCommand(COMMAND_REQUEST_STATUS);
        }
        else
            bRender = true; //So we see the "disconnected" screen.
        
        sleep(1);
    }
    
    pthread_exit(NULL);
}

/* Functions. */
void ReceiveStatus(uint8_t* pStatus, uint32_t lLength)
{
    memcpy(&status, pStatus, lLength);
    
    //Determine if we should request a state change.
    switch(nSwitchRequest)
    {
        case SYSTEM_STATE_DAY:
        {
            //Cancel any request if it's in day (battery) mode or night (grid) mode.
            if(SYSTEM_STATE_DAY == status.nSystemState || SYSTEM_STATE_NIGHT == status.nSystemState)
            {
                printf("Cancelling batt switch request as it's already on batts/night.\n");
                nSwitchRequest = SYSTEM_STATE_NO_CHANGE;
            }
            else
            {
                //Send a request to switch to batteries.
                printf("Requesting batts...\n");
                tcpclient_SendCommand(COMMAND_REQUEST_BATTS);
            }
        }
        break;
        
        case SYSTEM_STATE_BYPASS:
        {
            //Cancel any request if it's in bypass (grid) mode or night (grid) mode.
            if(SYSTEM_STATE_BYPASS == status.nSystemState || SYSTEM_STATE_NIGHT == status.nSystemState)
            {
                printf("Cancelling grid switch request as it's already on grid/night.\n");
                nSwitchRequest = SYSTEM_STATE_NO_CHANGE;
            }
            else
            {
                //Send a request to switch to batteries.
                printf("Requesting grid...\n");
                tcpclient_SendCommand(COMMAND_REQUEST_GRID);
            }
        }
        break;
    }
    
    bRender = true;
}

void WindowSizeChanged(int newWidth, int newHeight)
{
    lXWindowSize = newWidth;
    lYWindowSize = newHeight;

    for(int i = 0; i < widgetCount; i++)
    {
        Widget_ScreenChanged(Widgets[i], newWidth, newHeight);
    }
}

void Clicked(int lX, int lY)
{
    for(int i = 0; i < widgetCount; i++)
    {
        Widget_Click(Widgets[i], lX, lY);
    }
    
    bRender = true;
}

int main(int argc, char* argv[])
{
    widgetCount = sizeof(Widgets)/sizeof(struct sdfWidget*);
    
    BatteryWidget_Initialise(&sdcWidgetBattery,
                             0.01f,
                             0.01f,
                             0.15f,
                             0.98f,
                             &status);
    
    GaugeWidget_Initialise(&sdcWidgetPowerGauge,
                           0.2f,
                           0.01f,
                           0.5f,
                           0.98f,
                           0.0f,
                           1100.0f,
                           20.0f,
                           200.0f,
                           999.0f,
                           -155.0f * M_PI / 180.0,
                           -25.0f * M_PI / 180.0,
                           &status.nLoadPercent,
                           "W");

    StatusSwitchWidget_Initialise(&sdcWidgetStatusSwitch,
                                  0.79f,
                                  0.01f,
                                  0.2f,
                                  0.98f,
                                  &status,
                                  &nSwitchRequest);

    if(!tcpclient_init())
    {
        printf("Failed to create TCP client. Exiting.\n");    
        return -1;
    }
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window;
    
    if(sizeof(void*) == 4)
    {
        //On Raspberry Pi. Launch full screen.
        window = SDL_CreateWindow("Inverter Gubbin",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  0,
                                  0,
                                  SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    else
    {
        //On dev machine. Launch a window.
        window = SDL_CreateWindow("Inverter Gubbin",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  800,
                                  480,
                                  SDL_WINDOW_SHOWN);
                                  
        WindowSizeChanged(800, 480);
    }
    
    if (!window)
    {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return 1;
    }
    
    if(0 != pthread_create(&processingThread, NULL, &process, NULL))
    {
        printf("Error creating processing thread\n");
        return 1;
    }

    UIUtils_Init();

    SDL_Event e;

    while (!quit)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            switch(e.type)
            {
                case SDL_QUIT:
                {
                    quit = true;
                }
                break;
                
                case SDL_KEYDOWN:
                {
                    if (e.key.keysym.sym == SDLK_ESCAPE)
                    {
                        quit = true;
                    }
                }
                break;
                
                case SDL_WINDOWEVENT:
                {
                    if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        WindowSizeChanged(e.window.data1, e.window.data2);
                    }
                }
                
                case SDL_FINGERDOWN:
                {
                    if(SDL_GetTicks() > UI_INPUT_FILTER)
                    {
                        Clicked((int)(e.tfinger.x * (float)lXWindowSize), (int)(e.tfinger.y * (float)lYWindowSize));
                    }
                }
                break;
                
                case SDL_MOUSEBUTTONDOWN:
                {
                    if(SDL_GetTicks() > UI_INPUT_FILTER)
                    {
                        Clicked(e.button.x, e.button.y);
                    }
                }
                break;
            }
        
            if (e.type == SDL_QUIT)
            {
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
            }
        }
        
        if(bRender)
        {
            bRender = false;
            
            //Render.
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            
            if(tcpclient_GetConnected())
            {
                for(int i = 0; i < widgetCount; i++)
                {
                    Widget_Update(Widgets[i], renderer);
                }
            }
            else
            {
                SDL_Rect rectNotConnectedText = { lXWindowSize / 4,
                                                  lYWindowSize / 2,
                                                  lXWindowSize / 2,
                                                  lYWindowSize / 10 };
            
                UIUtils_RenderText(renderer, colText, rectNotConnectedText, "Not Connected");
            }
            
            SDL_RenderPresent(renderer);
        }

        SDL_Delay(50);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
