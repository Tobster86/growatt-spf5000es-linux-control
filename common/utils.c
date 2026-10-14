
#include "utils.h"

uint16_t utils_GetOffpeakChargingAmps(struct SystemStatus* pSystemStatus)
{
    //Intelligent charging calculation.
    uint16_t nResult = GW_CFG_UTIL_AMPS_MOD;

    /*if(pSystemStatus->nOffPeakChargeKwh > 0) //Don't do this unless we've measured an off-peak charge energy.
    {
        float fltBoostChargeEnergy = (float)(pSystemStatus->nAcchgegyToday - pSystemStatus->nOffPeakChargeKwh) *
                                     GW_WORST_CASE_CHARGE_EFFICIENCY;
        float fltBattUseEnergy = (float)(pSystemStatus->nBattuseToday / GW_WORST_CASE_CHARGE_EFFICIENCY);
        float fltSolarEnergy = (float)pSystemStatus->nSolarToday;
                
        //If more was charged than discharge, go with lower cap (sidestep wrapping issues).
        if(fltBoostChargeEnergy > fltBattUseEnergy)
        {
            nResult = CHARGE_MIN_AMPS;
        }
        else
        {
            float fltEnergyToCharge = (fltBattUseEnergy - fltBoostChargeEnergy - fltSolarEnergy) * GW_WH_MULTIPLIER;
            float fltChargeCurrent = fltEnergyToCharge / CHARGE_VOLTAGE / CHARGE_HOURS;
        
            //Charge current is shared by the inverters.
            if(fltEnergyToCharge > 0)
                nResult = (uint16_t)((fltChargeCurrent / (float)INVERTER_COUNT) + 0.5f);
            else
                nResult = 0;
        }
    }
    
    //Sanity checks, spreading sane current values across the inverters.
    if(nResult > (GW_CFG_UTIL_AMPS_MAX / INVERTER_COUNT))
        nResult = (GW_CFG_UTIL_AMPS_MAX / INVERTER_COUNT);
    else if(nResult < (CHARGE_MIN_AMPS / INVERTER_COUNT))
        nResult = (CHARGE_MIN_AMPS / INVERTER_COUNT);*/
    
    return nResult;
}

