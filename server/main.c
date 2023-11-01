
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

#define SWITCH_TO_GRID_LOAD_PERCENT 950 //The raw value is actually permille.

bool bRunning = true;

enum ModbusState
{
    INIT,
    PROCESS,
    DEINIT,
    DIE
};

enum ModbusState modbusState = INIT;
modbus_t *ctx;

struct SystemStatus status;
uint16_t nInverterMode;
bool bManualSwitchToGrid = false;
bool bManualSwitchToBatts = false;

void* network_thread(void* arg)
{
}

static void reinit()
{
    printf("MODBUS comms reinit.\n");

    if(ctx != NULL)
    {
        modbus_close(ctx);
        modbus_free(ctx);
    }

    modbusState = INIT;
    sleep(1);
}

void* modbus_thread(void* arg)
{
    uint16_t inputRegs[INPUT_REGISTER_COUNT];

    while(modbusState != DIE)
    {
        switch(modbusState)
        {
            case INIT:
            {
                modbusState = PROCESS;

                //Create a MODBUS context on /dev/ttyXRUSB0
                ctx = modbus_new_rtu("/dev/ttyXRUSB0", 9600, 'N', 8, 1);
                if (ctx == NULL)
                {
                    reinit();
                }

                //Connect to the MODBUS slave (slave ID 1)
                modbus_set_slave(ctx, 1);
                if (modbus_connect(ctx) == -1)
                {
                    reinit();
                }
            }
            break;

            case PROCESS:
            {
                //Read input registers and holding register (inverter mode).
                if(-1 == modbus_read_input_registers(ctx, 0, INPUT_REGISTER_COUNT, inputRegs) ||
                   -1 == modbus_read_registers(ctx, GW_HREG_CFG_MODE, 1, &nInverterMode))
                {
                    reinit();
                }
                else
                {
                    int write_rc = 0;
                
                    //Get the time.
                    time_t rawtime;
                    struct tm* timeinfo;
                    time(&rawtime);
                    timeinfo = localtime(&rawtime);
                
                    //Store relevant input register values.
                    status.nOutputWatts = inputRegs[OUTPUT_WATTS_L];
                    status.nOutputApppwr = inputRegs[OUTPUT_APPPWR_L];
                    status.nAcChargeWattsH = inputRegs[AC_CHARGE_WATTS_H];
                    status.nAcChargeWattsL = inputRegs[AC_CHARGE_WATTS_L];
                    status.nBatteryVolts = inputRegs[BATTERY_VOLTS];
                    status.nBusVolts = inputRegs[BUS_VOLTS];
                    status.nGridVolts = inputRegs[GRID_VOLTS];
                    status.nGridFreq = inputRegs[GRID_FREQ];
                    status.nAcOutVolts = inputRegs[AC_OUT_VOLTS];
                    status.nAcOutFreq = inputRegs[AC_OUT_FREQ];
                    status.nDcOutVolts = inputRegs[DC_OUT_VOLTS];
                    status.nInverterTemp = inputRegs[INVERTER_TEMP];
                    status.nDCDCTemp = inputRegs[DCDC_TEMP];
                    status.nLoadPercent = inputRegs[LOAD_PERCENT];
                    status.nBattPortVolts = inputRegs[BATT_PORT_VOLTS];
                    status.nBattBusVolts = inputRegs[BATT_BUS_VOLTS];
                    status.nBuck1Temp = inputRegs[BUCK1_TEMP];
                    status.nBuck2Temp = inputRegs[BUCK2_TEMP];
                    status.nOutputAmps = inputRegs[OUTPUT_AMPS];
                    status.nInverterAmps = inputRegs[INVERTER_AMPS];
                    status.nAcInputWattsH = inputRegs[AC_INPUT_WATTS_H];
                    status.nAcInputWattsL = inputRegs[AC_INPUT_WATTS_L];
                    status.nAcchgegyToday = inputRegs[ACCHGEGY_TODAY_L];
                    status.nBattuseToday = inputRegs[BATTUSE_TODAY_L];
                    status.nAcUseToday = inputRegs[AC_USE_TODAY_L];
                    status.nAcBattchgAmps = inputRegs[AC_BATTCHG_AMPS];
                    status.nAcUseWatts = inputRegs[AC_USE_WATTS_L];
                    status.nBattuseWatts = inputRegs[BATTUSE_WATTS_L];
                    status.nBattWatts = inputRegs[BATT_WATTS_L];
                    status.nMpptFanspeed = inputRegs[MPPT_FANSPEED];
                    status.nInvFanspeed = inputRegs[INV_FANSPEED];
                
                    //Auto/manual grid switching.
                    if(SYSTEM_STATE_DAY == status.nSystemState &&
                       (status.nLoadPercent > SWITCH_TO_GRID_LOAD_PERCENT || bManualSwitchToGrid))
                    {
                        bManualSwitchToGrid = false;
                        status.nSystemState = SYSTEM_STATE_BYPASS;
                        nInverterMode = GW_CFG_MODE_GRID;
                        write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, nInverterMode);
                        status.slSwitchTime = time(NULL);
                        printf("Switched to grid due to overload/override.\n");
                    }
                    
                    //Day/night switching.
                    if(SYSTEM_STATE_NIGHT == status.nSystemState)
                    {
                        //Currently night. Switch to day?
                        if((timeinfo->tm_hour < SYSTEM_NIGHT_H && timeinfo->tm_min < SYSTEM_NIGHT_M) ||
                           (timeinfo->tm_hour >= SYSTEM_DAY_H && timeinfo->tm_min >= SYSTEM_NIGHT_M))
                        {
                            status.nSystemState = SYSTEM_STATE_DAY;
                            nInverterMode = GW_CFG_MODE_BATTS;
                            write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, nInverterMode);
                            printf("Switched to day.\n");
                        }
                    }
                    else
                    {
                        //Currently day/bypassed. Switch to night?
                        if((timeinfo->tm_hour >= SYSTEM_NIGHT_H && timeinfo->tm_min >= SYSTEM_NIGHT_M) ||
                           (timeinfo->tm_hour < SYSTEM_DAY_H && timeinfo->tm_min < SYSTEM_NIGHT_M))
                        {
                            status.nSystemState = SYSTEM_STATE_NIGHT;
                            nInverterMode = GW_CFG_MODE_GRID;
                            write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, nInverterMode);
                            printf("Switched to night.\n");
                        }
                    }
                    
                    //Auto/manual batts switching.
                    if(SYSTEM_STATE_BYPASS == status.nSystemState &&
                       ((status.nLoadPercent <= SWITCH_TO_GRID_LOAD_PERCENT &&
                         time(NULL) > status.slSwitchTime + RETURN_TO_BATTS_TIME) ||
                        bManualSwitchToBatts))
                    {
                        bManualSwitchToBatts = false;
                        status.nSystemState = SYSTEM_STATE_DAY;
                        nInverterMode = GW_CFG_MODE_BATTS;
                        write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, nInverterMode);
                        printf("Switched to batts due to overload expiry/override.\n");
                    }
                    
                    //Final state check. (Note: inverter mode is read back from inverter on next pass).
                    switch(status.nSystemState)
                    {
                        case SYSTEM_STATE_DAY:
                        {
                            if(GW_CFG_MODE_BATTS != nInverterMode)
                            {
                                nInverterMode = GW_CFG_MODE_BATTS;
                                write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, nInverterMode);
                                printf("Wasn't on batts as expected. Rewrote holding register.\n");
                            }
                        }
                        break;
                        
                        case SYSTEM_STATE_BYPASS:
                        case SYSTEM_STATE_NIGHT:
                        {
                            if(GW_CFG_MODE_GRID != nInverterMode)
                            {
                                nInverterMode = GW_CFG_MODE_GRID;
                                write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, nInverterMode);
                                printf("Wasn't on grid as expected. Rewrote holding register.\n");
                            }
                        }
                        break;
                    }
                    
                    if(write_rc < 0)
                    {
                        printf("A holding register write failed.\n");
                        reinit();
                    }
                }
            }
            break;

            case DEINIT:
            {
                modbus_close(ctx);
                modbus_free(ctx);
                modbusState = DIE;
            }
            break;

            default: {}
        }
    }

    return NULL;
}

int main()
{  
    char input;
    pthread_t thread_modbus;
    
    memset(&status, 0x00, sizeof(struct SystemStatus));

    if (pthread_create(&thread_modbus, NULL, modbus_thread, NULL) != 0)
    {
        printf("Error creating modbus thread.\n");
        return 1;
    }
    
    while (bRunning)
    {
        input = getchar();

        switch (input)
        {
            case 'q':
            {
                bRunning = false;
                modbusState = DEINIT;
            }
            break;

            case 's':
            {
                printf("---=== Status ===---\n");
                printf("nSystemState\t%d\n", status.nSystemState);
                printf("nModbusFPS\t%d\n", status.nModbusFPS);
                printf("nInverterState\t%d\n", status.nInverterState);
                printf("nOutputWatts\t%d\n", status.nOutputWatts);
                printf("nOutputApppwr\t%d\n", status.nOutputApppwr);
                printf("nAcChargeWattsH\t%d\n", status.nAcChargeWattsH);
                printf("nAcChargeWattsL\t%d\n", status.nAcChargeWattsL);
                printf("nBatteryVolts\t%d\n", status.nBatteryVolts);
                printf("nBusVolts\t%d\n", status.nBusVolts);
                printf("nGridVolts\t%d\n", status.nGridVolts);
                printf("nGridFreq\t%d\n", status.nGridFreq);
                printf("nAcOutVolts\t%d\n", status.nAcOutVolts);
                printf("nAcOutFreq\t%d\n", status.nAcOutFreq);
                printf("nDcOutVolts\t%d\n", status.nDcOutVolts);
                printf("nInverterTemp\t%d\n", status.nInverterTemp);
                printf("nDCDCTemp\t%d\n", status.nDCDCTemp);
                printf("nLoadPercent\t%d\n", status.nLoadPercent);
                printf("nBattPortVolts\t%d\n", status.nBattPortVolts);
                printf("nBattBusVolts\t%d\n", status.nBattBusVolts);
                printf("nBuck1Temp\t%d\n", status.nBuck1Temp);
                printf("nBuck2Temp\t%d\n", status.nBuck2Temp);
                printf("nOutputAmps\t%d\n", status.nOutputAmps);
                printf("nInverterAmps\t%d\n", status.nInverterAmps);
                printf("nAcInputWattsH\t%d\n", status.nAcInputWattsH);
                printf("nAcInputWattsL\t%d\n", status.nAcInputWattsL);
                printf("nAcchgegyToday\t%d\n", status.nAcchgegyToday);
                printf("nBattuseToday\t%d\n", status.nBattuseToday);
                printf("nAcUseToday\t%d\n", status.nAcUseToday);
                printf("nAcBattchgAmps\t%d\n", status.nAcBattchgAmps);
                printf("nAcUseWatts\t%d\n", status.nAcUseWatts);
                printf("nBattuseWatts\t%d\n", status.nBattuseWatts);
                printf("nBattWatts\t%d\n", status.nBattWatts);
                printf("nMpptFanspeed\t%d\n", status.nMpptFanspeed);
                printf("nInvFanspeed\t%d\n", status.nInvFanspeed);
                printf("--------------------\n");
            }
            break;

            default:
            {
                printf("---=== %c unknown - available Commands ===---\n", input);
                printf("q - Quit\n");
                printf("s - Print latest status\n");
                printf("--------------------------------\n");
                break;
            }
        }
    }
    
    printf("Waiting for threads to finish...\n");
    pthread_join(thread_modbus, NULL);
    printf("...MODBUS done...\n");
    printf("Done.\n");
    return 0;
}





























