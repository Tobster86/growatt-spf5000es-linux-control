
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
#include "GaugeWidget.h"

struct SystemStatus status;

struct sdfWidget sdcWidgetPowerGauge;

struct sdfWidget* Widgets[] =
{
    &sdcWidgetPowerGauge
};

int widgetCount;

void ReceiveStatus(uint8_t* pStatus, uint32_t lLength)
{
    printf("Received status.\n");
        
    memcpy(&status, pStatus, lLength);
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
    
    GaugeWidget_Initialise(&sdcWidgetPowerGauge,
                           0.0f,
                           0.0f,
                           0.3f,
                           1.0f,
                           5000.0f,
                           0.0f);
    
    /*if(!tcpclient_init())
    {
        printf("Failed to create TCP client. Exiting.\n");
    
        return -1;
    }

    while(true)
    {
        if(tcpclient_GetConnected())
        {
            printf("Sending a command...\n");
            tcpclient_SendCommand(COMMAND_REQUEST_STATUS);
        }
        else
        {
            printf("Not sending a command as we're not connected.\n");
        }
        
        sleep(1);
    }*/


    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Changing Background Color",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          0,
                                          0,
                                          SDL_WINDOW_FULLSCREEN_DESKTOP);
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

    // Seed the random number generator
    srand(time(NULL));

    bool quit = false;
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

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
