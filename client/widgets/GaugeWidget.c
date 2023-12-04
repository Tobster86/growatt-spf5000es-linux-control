
#include "GaugeWidget.h"

const SDL_Colour colMinPip = { 100, 100, 100, 255 };
const SDL_Colour colMajPip = { 255, 255, 255, 255 };
const SDL_Colour colMinWarn = { 150, 0, 0, 255 };
const SDL_Colour colMajWarn = { 255, 0, 0, 255 };
const SDL_Colour colNeedle = { 0, 255, 255, 255 };

void GaugeWidget_Initialise(struct sdfWidget* psdcWidget,
                            float fltXOffset,
                            float fltYOffset,
                            float fltWidth,
                            float fltHeight,
                            float fltMin,
                            float fltMax,
                            float fltMinInc,
                            float fltMajInc,
                            float fltWarn,
                            float fltMinAng,
                            float fltMaxAng,
                            uint32_t* plValue)
{
    psdcWidget->psdcGaugeWidget = malloc(sizeof(struct sdfGaugeWidget));
    psdcWidget->psdcGaugeWidget->fltMin = fltMin;
    psdcWidget->psdcGaugeWidget->fltMax = fltMax;
    psdcWidget->psdcGaugeWidget->fltMajInc = fltMajInc;
    psdcWidget->psdcGaugeWidget->fltMinInc = fltMinInc;
    psdcWidget->psdcGaugeWidget->fltWarn = fltWarn;
    psdcWidget->psdcGaugeWidget->fltMinAng = fltMinAng;
    psdcWidget->psdcGaugeWidget->fltMaxAng = fltMaxAng;
    psdcWidget->psdcGaugeWidget->plValue = plValue;
    Widget_Initialise(psdcWidget, fltXOffset, fltYOffset, fltWidth, fltHeight, GaugeWidget_Update, GaugeWidget_ScreenChanged);
}

void GaugeWidget_Update(struct sdfWidget* psdcWidget, SDL_Renderer* pRenderer)
{
    float fltNeedleLength = fmin(psdcWidget->lWidth, psdcWidget->lHeight) * 0.4f;
    float fltMinorPipLength = fltNeedleLength * 0.05f;
    float fltMajorPipLength = fltNeedleLength * 0.1f;
    uint32_t lXOrigin = psdcWidget->lXPos + (psdcWidget->lWidth / 2);
    uint32_t lYOrigin = psdcWidget->lYPos + (psdcWidget->lHeight / 2);
    
    float fltMinIncAng = ((psdcWidget->psdcGaugeWidget->fltMaxAng - psdcWidget->psdcGaugeWidget->fltMinAng) /
                         (psdcWidget->psdcGaugeWidget->fltMax - psdcWidget->psdcGaugeWidget->fltMin)) *
                         psdcWidget->psdcGaugeWidget->fltMinInc;
    float fltMajIncAng = ((psdcWidget->psdcGaugeWidget->fltMaxAng - psdcWidget->psdcGaugeWidget->fltMinAng) /
                         (psdcWidget->psdcGaugeWidget->fltMax - psdcWidget->psdcGaugeWidget->fltMin)) *
                         psdcWidget->psdcGaugeWidget->fltMajInc;
                         
                         
                         
    float fltWarnAng = psdcWidget->psdcGaugeWidget->fltMinAng +
                       (((psdcWidget->psdcGaugeWidget->fltMaxAng - psdcWidget->psdcGaugeWidget->fltMinAng) /
                         (psdcWidget->psdcGaugeWidget->fltMax - psdcWidget->psdcGaugeWidget->fltMin)) *
                          psdcWidget->psdcGaugeWidget->fltWarn);
    
    //Draw minor increments.
    SDL_SetRenderDrawColor(pRenderer, colMinPip.r, colMinPip.g, colMinPip.b, colMinPip.a);
    
    float fltAng = psdcWidget->psdcGaugeWidget->fltMinAng;
    
    printf("Rendering...");
        
    while(fltAng <= psdcWidget->psdcGaugeWidget->fltMaxAng)
    {
        printf("cosf(fltAng): %.2f\n", cosf(fltAng));
    
        if(fltAng >= fltWarnAng)
            SDL_SetRenderDrawColor(pRenderer, colMinWarn.r, colMinWarn.g, colMinWarn.b, colMinWarn.a);
    
        SDL_RenderDrawLine(pRenderer,
                           lXOrigin + (uint32_t)(((fltNeedleLength - fltMinorPipLength) * cosf(fltAng)) + 0.5f),
                           lYOrigin + (uint32_t)(((fltNeedleLength - fltMinorPipLength) * sinf(fltAng)) + 0.5f),
                           lXOrigin + (uint32_t)(((fltNeedleLength) * cosf(fltAng)) + 0.5f),
                           lYOrigin + (uint32_t)(((fltNeedleLength) * sinf(fltAng)) + 0.5f));
                           
        fltAng += fltMinIncAng;
    }
    
    //Draw major increments.
    SDL_SetRenderDrawColor(pRenderer, colMajPip.r, colMajPip.g, colMajPip.b, colMajPip.a);
    fltAng = psdcWidget->psdcGaugeWidget->fltMinAng;
    
    while(fltAng <= psdcWidget->psdcGaugeWidget->fltMaxAng)
    {
        if(fltAng >= fltWarnAng)
            SDL_SetRenderDrawColor(pRenderer, colMajWarn.r, colMajWarn.g, colMajWarn.b, colMajWarn.a);
            
        SDL_RenderDrawLine(pRenderer,
                           lXOrigin + (uint32_t)(((fltNeedleLength - fltMajorPipLength) * cosf(fltAng)) + 0.5f),
                           lYOrigin + (uint32_t)(((fltNeedleLength - fltMajorPipLength) * sinf(fltAng)) + 0.5f),
                           lXOrigin + (uint32_t)(((fltNeedleLength) * cosf(fltAng)) + 0.5f),
                           lYOrigin + (uint32_t)(((fltNeedleLength) * sinf(fltAng)) + 0.5f));
                           
        fltAng += fltMajIncAng;
    }
    
    //Draw needle.
    float fltValAng = psdcWidget->psdcGaugeWidget->fltMinAng +
                      (((psdcWidget->psdcGaugeWidget->fltMaxAng - psdcWidget->psdcGaugeWidget->fltMinAng) /
                        (psdcWidget->psdcGaugeWidget->fltMax - psdcWidget->psdcGaugeWidget->fltMin)) *
                        (float)*psdcWidget->psdcGaugeWidget->plValue);
                       
    SDL_SetRenderDrawColor(pRenderer, colNeedle.r, colNeedle.g, colNeedle.b, colNeedle.a);
                       
    SDL_RenderDrawLine(pRenderer,
                       lXOrigin,
                       lYOrigin,
                       lXOrigin + (uint32_t)(((fltNeedleLength) * cosf(fltValAng)) + 0.5f),
                       lYOrigin + (uint32_t)(((fltNeedleLength) * sinf(fltValAng)) + 0.5f));
}

void GaugeWidget_ScreenChanged(struct sdfWidget* psdcWidget)
{
}

