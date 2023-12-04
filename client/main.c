
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "system_defs.h"
#include "comms_defs.h"
#include "tcpclient.h"
#include "Widget.h"
#include "BatteryWidget.h"
#include "GaugeWidget.h"

#define BATT_CAPACITY_100WH     143.36f

/* System status and variables. */
struct SystemStatus status;
uint32_t lBatteryPercent;
uint32_t lLoadPercent;

/* UI Stuff. */
struct sdfWidget sdcWidgetBattery;
struct sdfWidget sdcWidgetPowerGauge;

struct sdfWidget* Widgets[] =
{
    &sdcWidgetBattery,
    &sdcWidgetPowerGauge
};

int widgetCount;
bool bRender = true;

/* System logic */
pthread_t processingThread;
bool quit = false;

void *process(void *arg)
{
    while(!quit)
    {
        if(tcpclient_GetConnected())
        {
            tcpclient_SendCommand(COMMAND_REQUEST_STATUS);
        }
        
        sleep(1);
    }
    
    pthread_exit(NULL);
}

/* Functions. */
void ReceiveStatus(uint8_t* pStatus, uint32_t lLength)
{
    memcpy(&status, pStatus, lLength);
    
    //Calculate battery percent.
    lBatteryPercent = (uint32_t)((((BATT_CAPACITY_100WH - (float)status.nBattuseToday) / BATT_CAPACITY_100WH) * 100.0f) + 0.5f);
    
    lLoadPercent = status.nLoadPercent;
    
    bRender = true;
}

void WindowSizeChanged(int newWidth, int newHeight)
{
    for(int i = 0; i < widgetCount; i++)
    {
        Widget_ScreenChanged(Widgets[i], newWidth, newHeight);
    }
}

int main(int argc, char* argv[])
{
    widgetCount = sizeof(Widgets)/sizeof(struct sdfWidget*);
    
    BatteryWidget_Initialise(&sdcWidgetBattery,
                             0.01f,
                             0.01f,
                             0.15f,
                             0.99f,
                             &lBatteryPercent);
    
    GaugeWidget_Initialise(&sdcWidgetPowerGauge,
                           0.2f,
                           0.01f,
                           0.5f,
                           0.99f,
                           0.0f,
                           1100.0f,
                           20.0f,
                           200.0f,
                           999.0f,
                           -155.0f * M_PI / 180.0,
                           -25.0f * M_PI / 180.0,
                           &lLoadPercent);
    
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
            
            for(int i = 0; i < widgetCount; i++)
            {
                Widget_Update(Widgets[i], renderer);
            }
            
            SDL_RenderPresent(renderer);
            SDL_Delay(1);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
