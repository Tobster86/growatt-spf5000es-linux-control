
#ifndef SYSTEM_DEFS_H
#define SYSTEM_DEFS_H

#include <stdint.h>

#define SYSTEM_NIGHT_H  23
#define SYSTEM_NIGHT_M  30
#define SYSTEM_DAY_H    5
#define SYSTEM_DAY_M    30

#define RETURN_TO_BATTS_TIME 3600 /* One hour in seconds to return back to batteries after override/overload */

#define SYSTEM_STATE_DAY    0 /* Running on batteries */
#define SYSTEM_STATE_BYPASS 1 /* Temporarily grid-switched */
#define SYSTEM_STATE_NIGHT  2 /* Grid-switched and charging batteries */

struct SystemStatus
{
    //Program status.
    uint16_t nSystemState;
    uint16_t nModbusFPS;
    int32_t  slSwitchTime;
    
    //Inverter status.
    uint16_t nInverterState;
    uint16_t nOutputWatts;
    uint16_t nOutputApppwr; //Slightly higher than output watts at idle. Load + inverter?
    uint16_t nAcChargeWattsH; //Appears when charging?
    uint16_t nAcChargeWattsL; //Appears when charging?
    uint16_t nBatteryVolts;
    uint16_t nBusVolts;
    uint16_t nGridVolts;
    uint16_t nGridFreq;
    uint16_t nAcOutVolts;
    uint16_t nAcOutFreq;
    uint16_t nDcOutVolts;   //Appears when charging?
    uint16_t nInverterTemp;
    uint16_t nDCDCTemp;
    uint16_t nLoadPercent;
    uint16_t nBattPortVolts; //Appears when charging?
    uint16_t nBattBusVolts; //Appears when charging?
    uint16_t nBuck1Temp;
    uint16_t nBuck2Temp;
    uint16_t nOutputAmps;
    uint16_t nInverterAmps; //Similar to output apppwr when compared to output amps. Load + inverter?
    uint16_t nAcInputWattsH; //Appears when grid connected?
    uint16_t nAcInputWattsL; //Appears when grid connected?
    uint16_t nAcchgegyToday;
    uint16_t nBattuseToday;
    uint16_t nAcUseToday;
    uint16_t nAcBattchgAmps; //Appears when charging?
    uint16_t nAcUseWatts;
    uint16_t nBattuseWatts;
    uint16_t nBattWatts;
    uint16_t nMpptFanspeed;
    uint16_t nInvFanspeed;
};

void GrowattInputRegsToSystem(struct Status* pStatus, uint16_t* inputRegs);

#endif

