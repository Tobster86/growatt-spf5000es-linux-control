
#include "UIUtils.h"

static TTF_Font* font = NULL;
static float fltWidthToHeight = 5.0f / 3.0f; /* Monospace humanist font */

void UIUtils_Init()
{
    TTF_Init();
    
    font = TTF_OpenFont("assets/FiraCode-Regular.ttf", 120);

    if(NULL == font)
    {
        printf("Failed to load font.\n");
    }
}

void UIUtils_RenderText(SDL_Renderer* renderer,
                        SDL_Colour colText,
                        SDL_Rect rectText,
                        char* pcText)
{
    if(NULL != font)
    {
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, pcText, colText);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_RenderCopy(renderer, textTexture, NULL, &rectText);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }
}

