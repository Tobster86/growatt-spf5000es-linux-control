
#include "utils.h"

uint16_t utils_GetOffpeakChargingAmps(struct SystemStatus* pSystemStatus)
{
    //Intelligent charging calculation.
    uint16_t nResult = GW_CFG_UTIL_AMPS_MOD;

    if(pSystemStatus->nOffPeakChargeKwh > 0) //Don't do this unless we've measured an off-peak charge energy.
    {
        float fltBoostChargeEnergy = (float)(pSystemStatus->nAcchgegyToday - pSystemStatus->nOffPeakChargeKwh) *
                                            GW_WORST_CASE_CHARGE_EFFICIENCY;
        
        float fltBattUseEnergy = (float)(pSystemStatus->nBattuseToday / GW_WORST_CASE_CHARGE_EFFICIENCY);
        
        //If more was charged than discharge, go with lower cap (sidestep wrapping issues).
        if(fltBoostChargeEnergy > fltBattUseEnergy)
        {
            nResult = CHARGE_MIN_AMPS;
        }
        else
        {
            float fltEnergyToCharge = (fltBattUseEnergy - fltBoostChargeEnergy) * GW_WH_MULTIPLIER;
            float fltChargeCurrent = fltEnergyToCharge / CHARGE_VOLTAGE / CHARGE_HOURS;
        
            nResult = (uint16_t)(fltChargeCurrent + 0.5f);
        }
    }
    
    //Sanity checks.
    if(nResult > GW_CFG_UTIL_AMPS_MAX)
        nResult = GW_CFG_UTIL_AMPS_MAX;
    else if(nResult < CHARGE_MIN_AMPS)
        nResult = CHARGE_MIN_AMPS;
    
    return nResult;
}

