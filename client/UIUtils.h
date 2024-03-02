
#ifndef UIUTILS_H
#define UIUTILS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

void UIUtils_Init();
void UIUtils_RenderText(SDL_Renderer* renderer,
                        SDL_Colour colText,
                        SDL_Rect rectText,
                        char* pcText);

#endif
