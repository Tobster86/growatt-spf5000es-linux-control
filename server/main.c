
#include <stdio.h>
#include <stdarg.h>
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
#include "tcpserver.h"

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
int32_t slModeWriteTime;

uint16_t nLastInverterMode = 0xFFFF;
uint16_t nLastSystemState = 0xFFFF;
uint16_t nLastInverterState = 0xFFFF;

void printft(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    // Get the current time
    time_t rawtime;
    struct tm* timeinfo;
    char timestamp[20]; // Adjust the size as needed

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Print the timestamp and the formatted message
    printf("[%s] ", timestamp);
    vprintf(format, args);

    va_end(args);
}

void GetStatus(uint8_t** ppStatus, uint32_t* pLength)
{
    *ppStatus = (uint8_t*)&status;
    *pLength = sizeof(struct SystemStatus);
}

void SetBatts()
{
    printft("Remote user requested switch to batts.\n");
    bManualSwitchToGrid = false;
    bManualSwitchToBatts = true;
}

void SetGrid()
{
    printft("Remote user requested switch to grid.\n");
    bManualSwitchToGrid = true;
    bManualSwitchToBatts = false;
}

static void reinit()
{
    printft("MODBUS comms reinit.\n");

    if(ctx != NULL)
    {
        modbus_close(ctx);
        modbus_free(ctx);
        ctx = NULL;
    }

    modbusState = INIT;
    sleep(5);
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
                
                printft("MODBUS initialised. Going to processing.\n");
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
                    int lHour = timeinfo->tm_hour;
                    int lMin = timeinfo->tm_min;
                
                    //Store relevant input register values.
                    status.nInverterState = inputRegs[STATUS];
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
                
                    //Manual grid switching.
                    if(SYSTEM_STATE_DAY == status.nSystemState && bManualSwitchToGrid)
                    {
                        bManualSwitchToGrid = false;
                        status.nSystemState = SYSTEM_STATE_BYPASS;
                        write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID);
                        slModeWriteTime = time(NULL);
                        printft("Switched to grid due to override.\n");
                    }
                    
                    //Day/night switching.
                    if ((lHour > 5 || (lHour == 5 && lMin >= 30)) && (lHour < 23 || (lHour == 23 && lMin <= 30)))
                    {
                        //Switch to day if night?
                        if(SYSTEM_STATE_NIGHT == status.nSystemState)
                        {
                            status.nSystemState = SYSTEM_STATE_DAY;
                            write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_BATTS);
                            slModeWriteTime = time(NULL);
                            printft("Switched to day.\n");
                        }
                    }
                    else
                    {
                        //Switch to night if day/bypassed?
                        if(SYSTEM_STATE_NIGHT != status.nSystemState)
                        {
                            status.nSystemState = SYSTEM_STATE_NIGHT;
                            write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID);
                            slModeWriteTime = time(NULL);
                            printft("Switched to night.\n");
                        }
                    }
                    
                    //Manual batts switching.
                    if(SYSTEM_STATE_BYPASS == status.nSystemState && bManualSwitchToBatts)
                    {
                        bManualSwitchToBatts = false;
                        status.nSystemState = SYSTEM_STATE_DAY;
                        write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_BATTS);
                        slModeWriteTime = time(NULL);
                        printft("Switched to batts due to override.\n");
                    }
                    
                    //Final state check. (Note: inverter mode is read back from inverter on next pass).
                    if(time(NULL) > slModeWriteTime + CHECK_MODE_TIMEOUT)
                    {
                        switch(status.nSystemState)
                        {
                            case SYSTEM_STATE_DAY:
                            {
                                if(GW_CFG_MODE_BATTS != nInverterMode)
                                {
                                    write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_BATTS);
                                    printft("Wasn't on batts as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                            
                            case SYSTEM_STATE_BYPASS:
                            case SYSTEM_STATE_NIGHT:
                            {
                                if(GW_CFG_MODE_GRID != nInverterMode)
                                {
                                    write_rc |= modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID);
                                    printft("Wasn't on grid as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                        }
                            
                        slModeWriteTime = time(NULL);
                    }
                    
                    //Print state changes.
                    if(nLastInverterMode != nInverterMode)
                    {
                        printft("nInverterMode changed to ");
                        switch(nInverterMode)
                        {
                            case GW_CFG_MODE_BATTS: printf("BATTERIES\n"); break;
                            case GW_CFG_MODE_GRID: printf("GRID\n"); break;
                            default: printf("GOD KNOWS! (%d)\n", nInverterMode); break;
                        }
                
                        nLastInverterMode = nInverterMode;
                    }
                    
                    if(nLastSystemState != status.nSystemState)
                    {
                        printft("nSystemState changed to ");
                        switch(status.nSystemState)
                        {
                            case SYSTEM_STATE_DAY: printf("DAY\n"); break;
                            case SYSTEM_STATE_BYPASS: printf("BYPASS\n"); break;
                            case SYSTEM_STATE_NIGHT: printf("NIGHT\n"); break;
                            default: printf("GOD KNOWS! (%d)\n", status.nSystemState); break;
                        }
                
                        nLastSystemState = status.nSystemState;
                    }
                    
                    if(nLastInverterState != status.nInverterState)
                    {
                        printft("nInverterState changed to ");
                        if(status.nInverterState >= 0 && status.nInverterState < INVERTER_STATE_COUNT)
                            printf("%s\n", GwInverterStatusStrings[status.nInverterState]);
                        else
                            printf("UNKNOWN (%d)\n", status.nInverterState);
                            
                        nLastInverterState = status.nInverterState;
                    }
                    
                    if(write_rc < 0)
                    {
                        printft("A holding register write failed.\n");
                        reinit();
                    }
                }
            }
            break;

            case DEINIT:
            {
                if(ctx != NULL)
                {
                    modbus_close(ctx);
                    modbus_free(ctx);
                    ctx = NULL;
                }
                
                modbusState = DIE;
                printft("MODBUS deinitialised.\n");
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

    if(tcpserver_init())
    {
        if (pthread_create(&thread_modbus, NULL, modbus_thread, NULL) == 0)
        {    
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
                        switch(nInverterMode)
                        {
                            case GW_CFG_MODE_BATTS: printf("nInverterMode\tBATTERIES\n"); break;
                            case GW_CFG_MODE_GRID: printf("nInverterMode\tGRID\n"); break;
                            default: printf("nInverterMode\tGOD KNOWS! (%d)\n", nInverterMode); break;
                        }
                        
                        switch(status.nSystemState)
                        {
                            case SYSTEM_STATE_DAY: printf("nSystemState\tDAY\n"); break;
                            case SYSTEM_STATE_BYPASS: printf("nSystemState\tBYPASS\n"); break;
                            case SYSTEM_STATE_NIGHT: printf("nSystemState\tNIGHT\n"); break;
                            default: printf("nSystemState\tGOD KNOWS! (%d)\n", status.nSystemState); break;
                        }
                        
                        if(status.nInverterState >= 0 && status.nInverterState < INVERTER_STATE_COUNT)
                            printf("nInverterState\t%s\n", GwInverterStatusStrings[status.nInverterState]);
                        else
                            printf("nInverterState\t UNKNOWN (%d)\n", status.nInverterState);
                        
                        printf("\n");
                        printf("slModeWriteTime\t%d\n", slModeWriteTime);
                        printf("The actual time\t%ld\n", time(NULL));
                        printf("slSwitchTime\t%d\n", status.slSwitchTime);
                        
                        printf("\n");
                        printf("nModbusFPS\t%d\n", status.nModbusFPS);
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
                    
                    case 'g':
                    {
                        printf("Forcing grid...\n");
                        bManualSwitchToGrid = true;
                    }
                    break;
                    
                    case 'b':
                    {
                        printf("Forcing batteries...\n");
                        bManualSwitchToBatts = true;
                    }
                    break;

                    default:
                    {
                        printf("---=== %c unknown - available Commands ===---\n", input);
                        printf("q - Quit\n");
                        printf("s - Print latest status\n");
                        printf("g - Manual grid mode\n");
                        printf("b - Manual battery mode\n");
                        printf("--------------------------------\n");
                        break;
                    }
                }
            }
        }
        else
        {
            printf("Error creating modbus thread.\n");
        }
    }
    else
    {
        printf("Error creating TCP server.\n");
    }
    
    printf("Waiting for threads to finish...\n");
    pthread_join(thread_modbus, NULL);
    printf("...MODBUS done...\n");
    tcpserver_deinit();
    printf("...TCP done. That's it.\n");
    return 0;
}





























