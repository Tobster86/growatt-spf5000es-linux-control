
#ifndef SYSTEM_DEFS_H
#define SYSTEM_DEFS_H

#include <stdint.h>

#define SYSTEM_START_OFF_PEAK_H  23
#define SYSTEM_START_OFF_PEAK_M  30
#define SYSTEM_END_OFF_PEAK_H    5
#define SYSTEM_END_OFF_PEAK_M    30

#define CHECK_MODE_TIMEOUT          10   //Number of seconds to check the mode after changing it.

#define SYSTEM_STATE_NO_CHANGE 0xFFFF /* Placeholder for clients not requesting a change. */
#define SYSTEM_STATE_PEAK      0      /* Running on batteries during peak time. */
#define SYSTEM_STATE_BYPASS    1      /* Temporarily grid-switched, not charging, peak time. */
#define SYSTEM_STATE_OFF_PEAK  2      /* Grid-switched and charging batteries during off-peak. */
#define SYSTEM_STATE_BOOST     3      /* Temporarily grid-switched, charging batteries, peak time. */

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

void GrowattInputRegsToSystem(struct SystemStatus* pStatus, uint16_t* inputRegs);

#endif

