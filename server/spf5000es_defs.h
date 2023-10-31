
#ifndef SPF5000ES_DEFS_H
#define SPF5000ES_DEFS_H

#define GW_HREG_CFG_MODE        1
#define GW_HREG_HO_OUTPUT_T_Y   45
#define GW_HREG_HO_OUTPUT_T_MO  46
#define GW_HREG_HO_OUTPUT_T_D   47
#define GW_HREG_HO_OUTPUT_T_H   48
#define GW_HREG_HO_OUTPUT_T_MI  49
#define GW_HREG_HO_OUTPUT_T_S   50
#define HOLDING_REGISTER_COUNT  51

#define GW_CFG_MODE_BATTS   0
#define GW_CFG_MODE_GRID    2

enum GwInputRegisters
{
    STATUS = 0,
    PV1_VOLTS, //unused
    PV2_VOLTS, //unused
    PV1_CHARGE_WATTS_H, //unused
    PV1_CHARGE_WATTS_L, //unused
    PV2_CHARGE_WATTS_H, //unused
    PV2_CHARGE_WATTS_L, //unused
    BUCK_1_AMPS, //unused
    BUCK_2_AMPS, //unused
    OUTPUT_WATTS_H, //unused
    OUTPUT_WATTS_L,
    OUTPUT_APPPWR_H, //unused
    OUTPUT_APPPWR_L,
    AC_CHARGE_WATTS_H,
    AC_CHARGE_WATTS_L,
    AC_CHARGE_VA_H, //unused
    AC_CHARGE_VA_L, //unused
    BATTERY_VOLTS,
    BATTERY_SOC, //unused
    BUS_VOLTS,
    GRID_VOLTS,
    GRID_FREQ,
    AC_OUT_VOLTS,
    AC_OUT_FREQ,
    DC_OUT_VOLTS,
    INVERTER_TEMP,
    DCDC_TEMP,
    LOAD_PERCENT,
    BATT_PORT_VOLTS,
    BATT_BUS_VOLTS,
    TOTAL_TIME_S_H, //unused
    TOTAL_TIME_S_L, //unused
    BUCK1_TEMP,
    BUCK2_TEMP,
    OUTPUT_AMPS,
    INVERTER_AMPS,
    AC_INPUT_WATTS_H,
    AC_INPUT_WATTS_L,
    AC_INPUT_VA_H, //unused
    AC_INPUT_VA_L, //unused
    FAULTBIT, //unused
    WARNBIT, //unused
    FAULTVALUE, //unused
    WARNVALUE, //unused
    DTC, //unused
    CHECKSTEP, //unused
    PRODLM, //unused
    CONSTPOKF, //unused
    EGY1GEN_TODAY_H, //unused
    EGY1GEN_TODAY_L, //unused
    EGY1GEN_TOTAL_H, //unused
    EGY1GEN_TOTAL_L, //unused
    EGY2GEN_TODAY_H, //unused
    EGY2GEN_TODAY_L, //unused
    EGY2GEN_TOTAL_H, //unused
    EGY2GEN_TOTAL_L, //unused
    ACCHGEGY_TODAY_H, //unused
    ACCHGEGY_TODAY_L,
    ACCHGEGY_TOTAL_H, //unused
    ACCHGEGY_TOTAL_L, //unused
    BATTUSE_TODAY_H, //unused
    BATTUSE_TODAY_L,
    BATTUSE_TOTAL_H, //unused
    BATTUSE_TOTAL_L, //unused
    AC_USE_TODAY_H, //unused
    AC_USE_TODAY_L,
    AC_USE_TOTAL_H, //unused
    AC_USE_TOTAL_L, //unused
    AC_BATTCHG_AMPS,
    AC_USE_WATTS_H, //unused
    AC_USE_WATTS_L,
    AC_USE_VA_H, //unused
    AC_USE_VA_L, //unused
    BATTUSE_WATTS_H, //unused
    BATTUSE_WATTS_L,
    BATTUSE_VA_H, //unused
    BATTUSE_VA_L, //unused
    BATT_WATTS_H, //unused
    BATT_WATTS_L,
    UNUSED, //unused
    BATT_OVERCHG, //unused
    MPPT_FANSPEED,
    INV_FANSPEED,
    INPUT_REGISTER_COUNT, //count
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
    PV_CHG_DISCHG
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

