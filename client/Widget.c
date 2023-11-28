
#include "Widget.h"

void Widget_Initialise(struct sdfWidget* psdcWidget,
                       float fltXOffset,
                       float fltYOffset,
                       float fltWidth,
                       float fltHeight,
                       Update update,
                       ScreenChanged screenChanged)
{
    memset(psdcWidget, 0x00, sizeof(struct sdfWidget));
    psdcWidget->fltXOffset = fltXOffset;
    psdcWidget->fltYOffset = fltYOffset;
    psdcWidget->fltWidth = fltWidth;
    psdcWidget->fltHeight = fltHeight;
    psdcWidget->update = update;
    psdcWidget->screenChanged = screenChanged;
}

void Widget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer)
{
    psdcWidget->update(psdcWidget, pRenderer);
}

void Widget_ScreenChanged(struct sdfWidget* psdcWidget, int lWidth, int lHeight)
{
    psdcWidget->lXPos = (uint32_t)((psdcWidget->fltXOffset * (float)lWidth) + 0.5f);
    psdcWidget->lYPos = (uint32_t)((psdcWidget->fltYOffset * (float)lHeight) + 0.5f);
    psdcWidget->lWidth = (uint32_t)((psdcWidget->fltWidth * (float)lWidth) + 0.5f);
    psdcWidget->lHeight = (uint32_t)((psdcWidget->fltHeight * (float)lHeight) + 0.5f);
    
    psdcWidget->screenChanged(psdcWidget);
}


