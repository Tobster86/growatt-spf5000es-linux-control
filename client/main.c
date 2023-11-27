
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

struct SystemStatus status;

void ReceiveStatus(uint8_t* pStatus, uint32_t lLength)
{
    printf("Received status.\n");
        
    memcpy(&status, pStatus, lLength);
}

int main(int argc, char* argv[])
{
    if(!tcpclient_init())
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
    }


    /*if (SDL_Init(SDL_INIT_VIDEO) < 0)
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

    Uint32 lastColorChange = 0;
    Uint32 colorChangeInterval = 1000;  // Change color every 1000 milliseconds (1 second)

    while (!quit)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
            }
        }

        // Check if it's time to change the background color
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastColorChange >= colorChangeInterval)
        {
            // Generate a new random background color
            SDL_SetRenderDrawColor(renderer, rand() % 256, rand() % 256, rand() % 256, 255);
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);

            // Update the last color change time
            lastColorChange = currentTime;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();*/

    return 0;
}
