
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

struct SystemStatus
{
    //Program status.
    uint16_t nSystemState;
    
    //Intelligent charging.
    uint16_t nOffPeakChargeKwh; //The total charge energy when the inverter switched to peak this morning.
    uint16_t nChargeCurrent;    //The charge current that should be employed.
        
    //Inverter status.
    uint16_t nInverterState;  //The inverter's current GwInverterStatus state.
    uint16_t nOutputWatts;    //Real time output load (excluding charging, including grid bypass).
    uint16_t nOutputApppwr;   //Output load but slightly higher. Includes non-charging inverter power?
    uint16_t nAcChargeWattsL; //Seems to be grid battery charging watts, but can register ~100W during grid bypass?
    uint16_t nBatteryVolts;   //Real time voltage of the batteries.
    uint16_t nBusVolts;       //Real time bus volts, nominally 39V-46V. Doesn't seem useful other than for monitoring.
    uint16_t nGridVolts;      //Real time incoming grid voltage, whether bypassing or not. Can detect external power cuts.
    uint16_t nGridFreq;       //Real time incoming grid frequency, in case it's not 50Hz?
    uint16_t nAcOutVolts;     //Real time output voltage, either from the grid bypass or the inverter itself.
    uint16_t nAcOutFreq;      //Real time output frequency, either from the grid bypass or the inverter itself.
    uint16_t nInverterTemp;   //Real time temperature of some part of the inverter.
    uint16_t nDCDCTemp;       //Real time temperature of another part of the inverter.
    uint16_t nLoadPercent;    //Real time total output load as a percentage of the inverter's capacity.
    uint16_t nBuck1Temp;      //And another.
    uint16_t nBuck2Temp;      //And another.
    uint16_t nOutputAmps;     //Real time output current from the inverter. Not grid, not charging.
    uint16_t nInverterAmps;   //Slightly higher than output amps, and includes charging.
    uint16_t nAcInputWattsL;  //Similar to AcChargeWatts but includes output loads.
    uint16_t nAcchgegyToday;  //Total kWh taken from the grid to charge batteries since midnight.
    uint16_t nBattuseToday;   //Total kWh taken from the batteries to power loads since midnight.
    uint16_t nAcUseToday;     //Total kWh taken from the grid to power non-charging loads since midnight.
    uint16_t nAcBattchgAmps;  //Real time battery charging DC current.
    uint16_t nAcUseWatts;     //Almost identical to OutputWatts. Presumably same thing measured elsewhere.
    uint16_t nBattUseWatts;   //Real time load on the batteries (not charging).
    uint16_t nBattWatts;      //Identical to BattUseWatts, but both directional (with charging).
    uint16_t nInvFanspeed;    //Real time inverter fan speed, as a percentage of max speed.
};

void GrowattInputRegsToSystem(struct SystemStatus* pStatus, uint16_t* inputRegs);

#endif

