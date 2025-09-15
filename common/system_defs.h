
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

#define CHARGE_VOLTAGE         56.0f
#define CHARGE_HOURS           6

#define CHARGE_MIN_AMPS        2      /* Absolute minimum charging amps to ever set. */

#define INVERTER_COUNT 1              /* How many inverters are in parallel. */
#define INVERTER_1_ID  1              /* The ID of the master inverter. It's assumed that subsequent ones increment from this.*/

struct SystemStatus
{
    //Program status.
    uint16_t nSystemState;
    
    //Intelligent charging.
    uint16_t nOffPeakChargeKwh;   //The total charge energy when the inverter switched to peak this morning.
    uint16_t nChargeCurrent;      //The charge current that should be employed.
    int32_t slOffPeakChgComplete; //The time at which overnight charging was deemed to be completed.
        
    //Inverter status.
    uint16_t nInverterState[INVERTER_COUNT];  //The inverter's current GwInverterStatus state.
    uint16_t nOutputWatts[INVERTER_COUNT];    //Real time output load (excluding charging, including grid bypass).
    uint16_t nOutputApppwr[INVERTER_COUNT];   //Output load but slightly higher. Includes non-charging inverter power?
    uint16_t nAcChargeWattsL[INVERTER_COUNT]; //Battery charging watts from the grid.
    uint16_t nBatteryVolts[INVERTER_COUNT];   //Voltage of the batteries.
    uint16_t nBusVolts[INVERTER_COUNT];       //Bus volts, nominally 39V-46V. For monitoring.
    uint16_t nGridVolts[INVERTER_COUNT];      //Incoming grid voltage, whether bypassing or not. Can detect external power cuts.
    uint16_t nGridFreq[INVERTER_COUNT];       //Incoming grid frequency, in case it's not 50Hz?
    uint16_t nAcOutVolts[INVERTER_COUNT];     //Output voltage, either from the grid bypass or the inverter itself.
    uint16_t nAcOutFreq[INVERTER_COUNT];      //Output frequency, either from the grid bypass or the inverter itself.
    uint16_t nInverterTemp[INVERTER_COUNT];   //Temperature of some part of the inverter.
    uint16_t nDCDCTemp[INVERTER_COUNT];       //Temperature of another part of the inverter.
    uint16_t nLoadPercent[INVERTER_COUNT];    //Total output load as a percentage of the inverter's capacity.
    uint16_t nBuck1Temp[INVERTER_COUNT];      //And another.
    uint16_t nBuck2Temp[INVERTER_COUNT];      //And another.
    uint16_t nOutputAmps[INVERTER_COUNT];     //Real time output current from the inverter. Not grid, not charging.
    uint16_t nInverterAmps[INVERTER_COUNT];   //Slightly higher than output amps, and includes charging.
    uint16_t nAcInputWattsL[INVERTER_COUNT];  //Similar to AcChargeWatts but includes output loads.
    uint16_t nAcchgegyToday[INVERTER_COUNT];  //Total kWh taken from the grid to charge batteries since midnight.
    uint16_t nBattuseToday[INVERTER_COUNT];   //Total kWh taken from the batteries to power loads since midnight.
    uint16_t nAcUseToday[INVERTER_COUNT];     //Total kWh taken from the grid to power non-charging loads since midnight.
    uint16_t nBattchgAmps[INVERTER_COUNT];    //Real time battery charging DC current.
    uint16_t nAcUseWatts[INVERTER_COUNT];     //Almost identical to OutputWatts. Presumably same thing measured elsewhere.
    uint16_t nBattUseWatts[INVERTER_COUNT];   //Real time load on the batteries (not charging).
    uint16_t nBattWatts[INVERTER_COUNT];      //Identical to BattUseWatts, but both directional (with charging).
    uint16_t nInvFanspeed[INVERTER_COUNT];    //Real time inverter fan speed, as a percentage of max speed.
};

void GrowattInputRegsToSystem(struct SystemStatus* pStatus, uint16_t* inputRegs);

#endif

