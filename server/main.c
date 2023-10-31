
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include <modbus.h>
#include <errno.h>

#include "spf5000es_defs.h"
#include "system_defs.h"

#define INPUT_REG_COUNT     83
#define HOLDING_REG_COUNT   51

long long current_time_millis()
{
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}

long long oTimer;
long long oTimeEnd;
uint32_t lCounts = 0;

struct Status status;

int main()
{  
    modbus_t *ctx;
    int rc;
    uint16_t tab_ireg[INPUT_REG_COUNT]; // To store the read data
    uint16_t tab_hreg[INPUT_REG_COUNT]; // To store the read data

    // Create a new Modbus context
    ctx = modbus_new_rtu("/dev/ttyXRUSB0", 9600, 'N', 8, 1);
    if (ctx == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return -1;
    }

    // Connect to the Modbus server (slave)
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    // Set the Modbus address of the remote device (slave)
    modbus_set_slave(ctx, 1); // Replace 1 with the appropriate slave address

    oTimer = current_time_millis();
    oTimeEnd = oTimer + 1000;
    
    while(oTimer < oTimeEnd)
    {
        rc = modbus_read_input_registers(ctx, STATUS, 1, &status.nInverterState);
        rc = modbus_read_input_registers(ctx, OUTPUT_WATTS_L, 1, &status.nOutputWatts);
        rc = modbus_read_input_registers(ctx, OUTPUT_APPPWR_L, 3, &status.nOutputApppwr);
        rc = modbus_read_input_registers(ctx, BATTERY_VOLTS, 1, &status.nBatteryVolts);
        rc = modbus_read_input_registers(ctx, BUS_VOLTS, 11, &status.nBusVolts);
        rc = modbus_read_input_registers(ctx, BUCK1_TEMP, 6, &status.nBuck1Temp);
        rc = modbus_read_input_registers(ctx, ACCHGEGY_TODAY_L, 1, &status.nAcchgegyToday);
        rc = modbus_read_input_registers(ctx, BATTUSE_TODAY_L, 1, &status.nBattuseToday);
        rc = modbus_read_input_registers(ctx, AC_USE_TODAY_L, 1, &status.nAcUseToday);
        rc = modbus_read_input_registers(ctx, AC_BATTCHG_AMPS, 1, &status.nAcBattchgAmps);
        rc = modbus_read_input_registers(ctx, AC_USE_WATTS_L, 1, &status.nAcUseWatts);
        rc = modbus_read_input_registers(ctx, BATTUSE_WATTS_L, 1, &status.nBattuseWatts);
        rc = modbus_read_input_registers(ctx, BATT_WATTS_L, 1, &status.nBattWatts);
        rc = modbus_read_input_registers(ctx, MPPT_FANSPEED, 2, &status.nMpptFanspeed);
    
        /*rc = modbus_read_input_registers(ctx, OUTPUT_WATTS_L, 1, &status.nOutputWatts);
        rc = modbus_read_input_registers(ctx, OUTPUT_APPPWR_L, 1, &status.nOutputApppwr);
        rc = modbus_read_input_registers(ctx, AC_CHARGE_WATTS_H, 1, &status.nAcChargeWattsH);
        rc = modbus_read_input_registers(ctx, AC_CHARGE_WATTS_L, 1, &status.nAcChargeWattsL);
        rc = modbus_read_input_registers(ctx, BATTERY_VOLTS, 1, &status.nBatteryVolts);
        rc = modbus_read_input_registers(ctx, BUS_VOLTS, 1, &status.nBusVolts);
        rc = modbus_read_input_registers(ctx, GRID_VOLTS, 1, &status.nGridVolts);
        rc = modbus_read_input_registers(ctx, GRID_FREQ, 1, &status.nGridFreq);
        rc = modbus_read_input_registers(ctx, AC_OUT_VOLTS, 1, &status.nAcOutVolts);
        rc = modbus_read_input_registers(ctx, AC_OUT_FREQ, 1, &status.nAcOutFreq);
        rc = modbus_read_input_registers(ctx, DC_OUT_VOLTS, 1, &status.nDcOutVolts);
        rc = modbus_read_input_registers(ctx, INVERTER_TEMP, 1, &status.nInverterTemp);
        rc = modbus_read_input_registers(ctx, DCDC_TEMP, 1, &status.nDCDCTemp);
        rc = modbus_read_input_registers(ctx, LOAD_PERCENT, 1, &status.nLoadPercent);
        rc = modbus_read_input_registers(ctx, BATT_PORT_VOLTS, 1, &status.nBattPortVolts);
        rc = modbus_read_input_registers(ctx, BATT_BUS_VOLTS, 1, &status.nBattBusVolts);
        rc = modbus_read_input_registers(ctx, BUCK1_TEMP, 1, &status.nBuck1Temp);
        rc = modbus_read_input_registers(ctx, BUCK2_TEMP, 1, &status.nBuck2Temp);
        rc = modbus_read_input_registers(ctx, OUTPUT_AMPS, 1, &status.nOutputAmps);
        rc = modbus_read_input_registers(ctx, INVERTER_AMPS, 1, &status.nInverterAmps);
        rc = modbus_read_input_registers(ctx, AC_INPUT_WATTS_H, 1, &status.nAcInputWattsH);
        rc = modbus_read_input_registers(ctx, AC_INPUT_WATTS_L, 1, &status.nAcInputWattsL);
        rc = modbus_read_input_registers(ctx, ACCHGEGY_TODAY_L, 1, &status.nAcchgegyToday);
        rc = modbus_read_input_registers(ctx, BATTUSE_TODAY_L, 1, &status.nBattuseToday);
        rc = modbus_read_input_registers(ctx, AC_USE_TODAY_L, 1, &status.nAcUseToday);
        rc = modbus_read_input_registers(ctx, AC_BATTCHG_AMPS, 1, &status.nAcBattchgAmps);
        rc = modbus_read_input_registers(ctx, AC_USE_WATTS_L, 1, &status.nAcUseWatts);
        rc = modbus_read_input_registers(ctx, BATTUSE_WATTS_L, 1, &status.nBattuseWatts);
        rc = modbus_read_input_registers(ctx, BATT_WATTS_L, 1, &status.nBattWatts);
        rc = modbus_read_input_registers(ctx, MPPT_FANSPEED, 1, &status.nMpptFanspeed);
        rc = modbus_read_input_registers(ctx, INV_FANSPEED, 1, &status.nInvFanspeed);*/
        
        printf("-------========= Regs =========-------\n");
        printf("nOutputWatts - %d\n", status.nOutputWatts);
        printf("nOutputApppwr - %d\n", status.nOutputApppwr);
        printf("nAcChargeWattsH - %d\n", status.nAcChargeWattsH);
        printf("nAcChargeWattsL - %d\n", status.nAcChargeWattsL);
        printf("nBatteryVolts - %d\n", status.nBatteryVolts);
        printf("nBusVolts - %d\n", status.nBusVolts);
        printf("nGridVolts - %d\n", status.nGridVolts);
        printf("nGridFreq - %d\n", status.nGridFreq);
        printf("nAcOutVolts - %d\n", status.nAcOutVolts);
        printf("nAcOutFreq - %d\n", status.nAcOutFreq);
        printf("nDcOutVolts - %d\n", status.nDcOutVolts);
        printf("nInverterTemp - %d\n", status.nInverterTemp);
        printf("nDCDCTemp - %d\n", status.nDCDCTemp);
        printf("nLoadPercent - %d\n", status.nLoadPercent);
        printf("nBattPortVolts - %d\n", status.nBattPortVolts);
        printf("nBattBusVolts - %d\n", status.nBattBusVolts);
        printf("nBuck1Temp - %d\n", status.nBuck1Temp);
        printf("nBuck2Temp - %d\n", status.nBuck2Temp);
        printf("nOutputAmps - %d\n", status.nOutputAmps);
        printf("nInverterAmps - %d\n", status.nInverterAmps);
        printf("nAcInputWattsH - %d\n", status.nAcInputWattsH);
        printf("nAcInputWattsL - %d\n", status.nAcInputWattsL);
        printf("nAcchgegyToday - %d\n", status.nAcchgegyToday);
        printf("nBattuseToday - %d\n", status.nBattuseToday);
        printf("nAcUseToday - %d\n", status.nAcUseToday);
        printf("nAcBattchgAmps - %d\n", status.nAcBattchgAmps);
        printf("nAcUseWatts - %d\n", status.nAcUseWatts);
        printf("nBattuseWatts - %d\n", status.nBattuseWatts);
        printf("nBattWatts - %d\n", status.nBattWatts);
        printf("nMpptFanspeed - %d\n", status.nMpptFanspeed);
        printf("nInvFanspeed - %d\n", status.nInvFanspeed);
        printf("--------------------------------------\n");

        /*if (rc == -1) {
            fprintf(stderr, "Read registers failed: %s\n", modbus_strerror(errno));
            modbus_free(ctx);
            return -1;
        }*/
        
        
/*  pStatus->nOutputWatts = inputRegs[OUTPUT_WATTS_L];
    pStatus->nOutputApppwr = inputRegs[OUTPUT_APPPWR_L];
    pStatus->nAcChargeWattsH = inputRegs[AC_CHARGE_WATTS_H];
    pStatus->nAcChargeWattsL = inputRegs[AC_CHARGE_WATTS_L];
    pStatus->nBatteryVolts = inputRegs[BATTERY_VOLTS];
    pStatus->nBusVolts = inputRegs[BUS_VOLTS];
    pStatus->nGridVolts = inputRegs[GRID_VOLTS];
    pStatus->nGridFreq = inputRegs[GRID_FREQ];
    pStatus->nAcOutVolts = inputRegs[AC_OUT_VOLTS];
    pStatus->nAcOutFreq = inputRegs[AC_OUT_FREQ];
    pStatus->nDcOutVolts = inputRegs[DC_OUT_VOLTS];
    pStatus->nInverterTemp = inputRegs[INVERTER_TEMP];
    pStatus->nDCDCTemp = inputRegs[DCDC_TEMP];
    pStatus->nLoadPercent = inputRegs[LOAD_PERCENT];
    pStatus->nBattPortVolts = inputRegs[BATT_PORT_VOLTS];
    pStatus->nBattBusVolts = inputRegs[BATT_BUS_VOLTS];
    pStatus->nBuck1Temp = inputRegs[BUCK1_TEMP];
    pStatus->nBuck2Temp = inputRegs[BUCK2_TEMP];
    pStatus->nOutputAmps = inputRegs[OUTPUT_AMPS];
    pStatus->nInverterAmps = inputRegs[INVERTER_AMPS];
    pStatus->nAcInputWattsH = inputRegs[AC_INPUT_WATTS_H];
    pStatus->nAcInputWattsL = inputRegs[AC_INPUT_WATTS_L];
    pStatus->nAcchgegyToday = inputRegs[ACCHGEGY_TODAY_L];
    pStatus->nBattuseToday = inputRegs[BATTUSE_TODAY_L];
    pStatus->nAcUseToday = inputRegs[AC_USE_TODAY_L];
    pStatus->nAcBattchgAmps = inputRegs[AC_BATTCHG_AMPS];
    pStatus->nAcUseWatts = inputRegs[AC_USE_WATTS_L];
    pStatus->nBattuseWatts = inputRegs[BATTUSE_WATTS_L];
    pStatus->nBattWatts = inputRegs[BATT_WATTS_L];
    pStatus->nMpptFanspeed = inputRegs[MPPT_FANSPEED];
    pStatus->nInvFanspeed = inputRegs[INV_FANSPEED];*/
    
        /*rc = modbus_read_input_registers(ctx, 0, INPUT_REG_COUNT, tab_ireg);
        if (rc == -1) {
            fprintf(stderr, "Read registers failed: %s\n", modbus_strerror(errno));
            modbus_free(ctx);
            return -1;
        }

        // Print the response
        for (int i = 0; i < INPUT_REG_COUNT; i++) {
            printf("Register %d: %d\n", i, tab_ireg[i]);
        }*/
        
        /*rc = modbus_read_registers(ctx, 0, HOLDING_REG_COUNT, tab_hreg);
        if (rc == -1) {
            fprintf(stderr, "Read registers failed: %s\n", modbus_strerror(errno));
            modbus_free(ctx);
            return -1;
        }

        // Print the response
        for (int i = 0; i < HOLDING_REG_COUNT; i++) {
            printf("Register %d: %d\n", i, tab_hreg[i]);
        }*/
        
        lCounts++;
        oTimer = current_time_millis();
    }
    
    printf("%d hits in %lldms.\n", lCounts, oTimer - oTimeEnd + 1000);

    // Disconnect and cleanup
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}





























