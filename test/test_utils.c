
#include "test.h"
#include "test_utils.h"
#include "utils.h"
#include "system_defs.h"
#include <string.h>
#include <stdbool.h>

/*
uint16_t utils_GetOffpeakChargingAmps(struct SystemStatus* pSystemStatus)
{
    //Intelligent charging calculation.
    uint16_t nResult = GW_CFG_UTIL_AMPS_MOD;

    if(pSystemStatus->nOffPeakChargeKwh > 0) //Don't do this unless we've measured an off-peak charge energy.
    {
        float fltBoostChargeEnergy = (float)(pSystemStatus->nAcchgegyToday - pSystemStatus->nOffPeakChargeKwh);
        float fltEnergyToCharge = (((float)pSystemStatus->nBattuseToday / GW_WORST_CASE_CHARGE_EFFICIENCY) -
                                  (fltBoostChargeEnergy * GW_WORST_CASE_CHARGE_EFFICIENCY)) * GW_WH_MULTIPLIER;
        float fltChargeCurrent = fltEnergyToCharge / CHARGE_VOLTAGE / CHARGE_HOURS;
        
        nResult = (uint16_t)(fltChargeCurrent + 0.5f);
    }
    
    //Sanity checks.
    if(nResult > GW_CFG_UTIL_AMPS_MAX)
        nResult = GW_CFG_UTIL_AMPS_MAX;
    else if(nResult < CHARGE_MIN_AMPS)
        nResult = CHARGE_MIN_AMPS;
    
    return nResult;
}
*/



static void test_utils_GetOffpeakChargingAmps()
{
    struct SystemStatus status;
    uint16_t nResult = 0;
    
    //No off-peak charge kWh measured.
    memset(&status, 0xAA, sizeof(struct SystemStatus));
    status.nOffPeakChargeKwh = 0;
    nResult = utils_GetOffpeakChargingAmps(&status);
    ASSERT_EQUAL(nResult, GW_CFG_UTIL_AMPS_MOD, "Default charging amps when no off-peak charge energy measured");
    
    //Garbage values.
    for (uint32_t i = 0x00000000; i <= 0x000000FF; i += 0x00000011)
    {
        memset(&status, (uint8_t)i, sizeof(struct SystemStatus));
        nResult = utils_GetOffpeakChargingAmps(&status);
        ASSERT_EQUAL(nResult >= CHARGE_MIN_AMPS && nResult <= GW_CFG_UTIL_AMPS_MAX,
                     true,
                     "Sensible value to write to the inverter from garbage (0x%02X): %d",
                     i,
                     nResult);
    }
    
    //No boost charging. 4.8kWh used. 6kWh to charge at 56V -> 18A.
    memset(&status, 0xAA, sizeof(struct SystemStatus));
    status.nOffPeakChargeKwh = 50;
    status.nAcchgegyToday = 50;
    status.nBattuseToday = 48;
    nResult = utils_GetOffpeakChargingAmps(&status);
    ASSERT_EQUAL(nResult, 18, "No boost charging, 4.8kWh used. 6kWh to charge at 56V -> 18A, = %d", nResult);
    
    //3.5kWh boost charging. 4.8kWh used. 3.2kWh to charge at 56V -> 10A.
    memset(&status, 0xAA, sizeof(struct SystemStatus));
    status.nOffPeakChargeKwh = 50;
    status.nAcchgegyToday = 85;
    status.nBattuseToday = 48;
    nResult = utils_GetOffpeakChargingAmps(&status);
    ASSERT_EQUAL(nResult, 10, "3.5kWh boost charging. 4.8kWh used. 3.2kWh to charge at 56V -> 10A, = %d", nResult);
    
    //Overnight/boost charging value weirdness not designed/tested for other than the basic sanity checks that cap min/max amps.
    
    //Next to nothing (200Wh) used. Capped at 2A.
    memset(&status, 0xAA, sizeof(struct SystemStatus));
    status.nOffPeakChargeKwh = 50;
    status.nAcchgegyToday = 50;
    status.nBattuseToday = 2;
    nResult = utils_GetOffpeakChargingAmps(&status);
    ASSERT_EQUAL(nResult, 2, "Next to nothing (200Wh) used. Capped at 2A, = %d", nResult);
    
    //Loads used. Capped at 80A.
    memset(&status, 0xAA, sizeof(struct SystemStatus));
    status.nOffPeakChargeKwh = 50;
    status.nAcchgegyToday = 50;
    status.nBattuseToday = 300;
    nResult = utils_GetOffpeakChargingAmps(&status);
    ASSERT_EQUAL(nResult, 80, "Loads used. Capped at 80A., = %d", nResult);
    
    //Lots used but met by boost charging. Capped at 2A.
    memset(&status, 0xAA, sizeof(struct SystemStatus));
    status.nOffPeakChargeKwh = 50;
    status.nAcchgegyToday = 350;
    status.nBattuseToday = 100;
    nResult = utils_GetOffpeakChargingAmps(&status);
    ASSERT_EQUAL(nResult, 2, "Lots used but met by boost charging. Capped at 2A, = %d", nResult);
}

void test_utils()
{
    PRINT_DEBUG("---=== Utils tests ===---\n");
    
    test_utils_GetOffpeakChargingAmps();
    
    PRINT_DEBUG("-------------------------\n\n");
}

