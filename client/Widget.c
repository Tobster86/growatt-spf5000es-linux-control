
#include "Widget.h"

void Widget_Initialise(struct sdfWidget* pWidget,
                       float fltXOffset,
                       float fltYOffset,
                       float fltWidth,
                       float fltHeight,
                       Update update,
                       ScreenChanged screenChanged)
{
    memset(pWidget, 0x00, sizeof(struct sdfWidget));
    pWidget->fltXOffset = fltXOffset;
    pWidget->fltYOffset = fltYOffset;
    pWidget->fltWidth = fltWidth;
    pWidget->fltHeight = fltHeight;
    pWidget->update = update;
    pWidget->screenChanged = screenChanged;
}

void Widget_ScreenChanged(struct sdfWidget* pWidget, int lWidth, int lHeight)
{
    pWidget->lXPos = (uint32_t)((pWidget->fltXOffset * (float)lWidth) + 0.5f);
    pWidget->lYPos = (uint32_t)((pWidget->fltYOffset * (float)lHeight) + 0.5f);
    pWidget->lWidth = (uint32_t)((pWidget->fltWidth * (float)lWidth) + 0.5f);
    pWidget->lHeight = (uint32_t)((pWidget->fltHeight * (float)lHeight) + 0.5f);
    
    pWidget->screenChanged(pWidget);
}

