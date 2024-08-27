
#ifndef SPF5000ES_DEFS_H
#define SPF5000ES_DEFS_H

//Growatt SPF ES Config holding register addresses.
#define GW_HREG_CFG_MODE        1
#define GW_HREG_UTIL_START_HOUR 5   //Hour from which utility charging allowed.
#define GW_HREG_UTIL_END_HOUR   6   //Hour til which utility charging allowed.
                                    //(Setting both to zero allows any time).
#define GW_HREG_MAX_CHG_AMPS    34  //Max = Utility + Solar according to manual
#define GW_HREG_MAX_UTIL_AMPS   38  //Max charging current from utility supply.
#define GW_HREG_TIME_Y          45
#define GW_HREG_TIME_MO         46
#define GW_HREG_TIME_D          47
#define GW_HREG_TIME_H          48
#define GW_HREG_TIME_MI         49
#define GW_HREG_TIME_S          50

//Growatt SPF ES Config values.
#define GW_CFG_MODE_BATTS   0
#define GW_CFG_MODE_GRID    2

enum GwInputRegisters
{
    STATUS = 0,
    PV1_VOLTS,
    PV2_VOLTS,
    PV1_CHARGE_WATTS_H,
    PV1_CHARGE_WATTS_L,
    PV2_CHARGE_WATTS_H,
    PV2_CHARGE_WATTS_L,
    BUCK_1_AMPS,
    BUCK_2_AMPS,
    OUTPUT_WATTS_H,
    OUTPUT_WATTS_L, //10
    OUTPUT_APPPWR_H,
    OUTPUT_APPPWR_L,
    AC_CHARGE_WATTS_H,
    AC_CHARGE_WATTS_L,
    AC_CHARGE_VA_H,
    AC_CHARGE_VA_L,
    BATTERY_VOLTS,
    BATTERY_SOC,
    BUS_VOLTS,
    GRID_VOLTS, //20
    GRID_FREQ,
    AC_OUT_VOLTS,
    AC_OUT_FREQ,
    DC_OUT_VOLTS,
    INVERTER_TEMP,
    DCDC_TEMP,
    LOAD_PERCENT,
    BATT_PORT_VOLTS,
    BATT_BUS_VOLTS,
    TOTAL_TIME_S_H, //30
    TOTAL_TIME_S_L,
    BUCK1_TEMP,
    BUCK2_TEMP,
    OUTPUT_AMPS,
    INVERTER_AMPS,
    AC_INPUT_WATTS_H,
    AC_INPUT_WATTS_L,
    AC_INPUT_VA_H,
    AC_INPUT_VA_L,
    FAULTBIT, //40
    WARNBIT,
    FAULTVALUE,
    WARNVALUE,
    DTC,
    CHECKSTEP,
    PRODLM,
    CONSTPOKF,
    EGY1GEN_TODAY_H,
    EGY1GEN_TODAY_L,
    EGY1GEN_TOTAL_H, //50
    EGY1GEN_TOTAL_L,
    EGY2GEN_TODAY_H,
    EGY2GEN_TODAY_L,
    EGY2GEN_TOTAL_H,
    EGY2GEN_TOTAL_L,
    ACCHGEGY_TODAY_H,
    ACCHGEGY_TODAY_L,
    ACCHGEGY_TOTAL_H ,
    ACCHGEGY_TOTAL_L,
    BATTUSE_TODAY_H, //60
    BATTUSE_TODAY_L,
    BATTUSE_TOTAL_H,
    BATTUSE_TOTAL_L,
    AC_USE_TODAY_H,
    AC_USE_TODAY_L,
    AC_USE_TOTAL_H,
    AC_USE_TOTAL_L,
    AC_BATTCHG_AMPS,
    AC_USE_WATTS_H,
    AC_USE_WATTS_L, //70
    AC_USE_VA_H,
    AC_USE_VA_L,
    BATTUSE_WATTS_H,
    BATTUSE_WATTS_L,
    BATTUSE_VA_H,
    BATTUSE_VA_L,
    BATT_WATTS_H,
    BATT_WATTS_L,
    UNUSED,
    BATT_OVERCHG, //80
    MPPT_FANSPEED,
    INV_FANSPEED,
    INPUT_REGISTER_COUNT
};

enum GwInverterStatus
{
    STANDBY = 0,
    NO_USE,
    DISCHARGE,
    FAULT,
    FLASH,
    PV_CHG,
    AC_CHG,
    COM_CHG,
    COM_CHG_BYP,
    PV_CHG_BYP,
    AC_CHG_BYP,
    BYPASS,
    PV_CHG_DISCHG,
    INVERTER_STATE_COUNT
};

const char* GwInverterStatusStrings[] =
{
    "STANDBY",
    "NO_USE",
    "DISCHARGE",
    "FAULT",
    "FLASH",
    "PV_CHG",
    "AC_CHG",
    "COM_CHG",
    "COM_CHG_BYP",
    "PV_CHG_BYP",
    "AC_CHG_BYP",
    "BYPASS",
    "PV_CHG_DISCHG"
};

//Holding Registers
#define HO_OUTPUT_CONFIG 1
#define HO_OUTPUT_T_Y 45
#define HO_OUTPUT_T_MO 46
#define HO_OUTPUT_T_D 47
#define HO_OUTPUT_T_H 48
#define HO_OUTPUT_T_MI 49
#define HO_OUTPUT_T_S 50

//Output configs
#define OUTPUT_CONFIG_BATTS 0
#define OUTPUT_CONFIG_UTIL 2

#endif

