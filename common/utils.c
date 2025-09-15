
#include "utils.h"

uint16_t utils_GetOffpeakChargingAmps(struct SystemStatus* pSystemStatus)
{
    //TO DO: Account for solar!

    //Intelligent charging calculation.
    uint16_t nResult = GW_CFG_UTIL_AMPS_MOD;

    if(pSystemStatus->nOffPeakChargeKwh > 0) //Don't do this unless we've measured an off-peak charge energy.
    {
        float fltBoostChargeEnergy = 0.0f;
        float fltBattUseEnergy = 0.0f;
        
        for(int i = 0; i < INVERTER_COUNT; i++)
        {
            fltBoostChargeEnergy += (float)(pSystemStatus->nAcchgegyToday[i] - pSystemStatus->nOffPeakChargeKwh) *
                                                GW_WORST_CASE_CHARGE_EFFICIENCY;
            
            fltBattUseEnergy += (float)(pSystemStatus->nBattuseToday[i] / GW_WORST_CASE_CHARGE_EFFICIENCY);
        }
        
        //If more was charged than discharge, go with lower cap (sidestep wrapping issues).
        if(fltBoostChargeEnergy > fltBattUseEnergy)
        {
            nResult = CHARGE_MIN_AMPS;
        }
        else
        {
            float fltEnergyToCharge = (fltBattUseEnergy - fltBoostChargeEnergy) * GW_WH_MULTIPLIER;
            float fltChargeCurrent = fltEnergyToCharge / CHARGE_VOLTAGE / CHARGE_HOURS;
        
            //Charge current is shared by the inverters.
            nResult = (uint16_t)((fltChargeCurrent / (float)INVERTER_COUNT) + 0.5f);
        }
    }
    
    //Sanity checks, spreading sane current values across the inverters.
    if(nResult > (GW_CFG_UTIL_AMPS_MAX / INVERTER_COUNT))
        nResult = (GW_CFG_UTIL_AMPS_MAX / INVERTER_COUNT);
    else if(nResult < (CHARGE_MIN_AMPS / INVERTER_COUNT))
        nResult = (CHARGE_MIN_AMPS / INVERTER_COUNT);
    
    return nResult;
}

