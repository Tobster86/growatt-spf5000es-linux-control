
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
bool bPrintConfigRegisters = false;

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

static void printft(const char* format, ...);

//Callbacks.
void _tcpserver_GetStatus(uint8_t** ppStatus, uint32_t* pLength)
{
    *ppStatus = (uint8_t*)&status;
    *pLength = sizeof(struct SystemStatus);
}

void _tcpserver_SetBatts()
{
    printft("Remote user requested switch to batts.\n");
    bManualSwitchToGrid = false;
    bManualSwitchToBatts = true;
}

void _tcpserver_SetGrid()
{
    printft("Remote user requested switch to grid.\n");
    bManualSwitchToGrid = true;
    bManualSwitchToBatts = false;
}

//Local functions.
static void printft(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    // Get the current time
    time_t rawtime;
    struct tm* timeinfo;
    char timestamp[20];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Print the timestamp and the formatted message
    printf("[%s] ", timestamp);
    vprintf(format, args);

    va_end(args);
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
    sleep(1);
}

static void SetOvernightAmps()
{
    if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, GW_CFG_UTIL_AMPS_MOD) < 0)
    {
        printft("Failed to write steady util charging amps to config register: %s\n", modbus_strerror(errno));
        reinit();
    }
}

static void SetBoostAmps()
{
    if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, GW_CFG_UTIL_AMPS_MAX) < 0)
    {
        printft("Failed to write max util charging amps to config register: %s\n", modbus_strerror(errno));
        reinit();
    }
}

static void DisallowOffPeakCharging()
{
    if(modbus_write_register(ctx, GW_HREG_UTIL_END_HOUR, GW_CFG_UTIL_TIME_OFFPEAK) < 0)
    {
        printft("Failed to write off-peak only charging config register: %s\n", modbus_strerror(errno));
        reinit();
    }
}

static void AllowOffPeakCharging()
{
    if(modbus_write_register(ctx, GW_HREG_UTIL_END_HOUR, GW_CFG_UTIL_TIME_ANY_TIME) < 0)
    {
        printft("Failed to write any time charging config register: %s\n", modbus_strerror(errno));
        reinit();
    }
}

static void SwitchToBypass()
{
    status.nSystemState = SYSTEM_STATE_BYPASS;
    slModeWriteTime = time(NULL);
    
    if(modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID) < 0)
    {
        printft("Failed to write grid mode (bypass) to config register: %s\n", modbus_strerror(errno));
        reinit();
    }
    
    //Enable the inverter's utility charging time limits to prevent unwanted charging.
    DisallowOffPeakCharging();
}

static void SwitchToPeak()
{
    status.nSystemState = SYSTEM_STATE_PEAK;
    slModeWriteTime = time(NULL);
    
    if(modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_BATTS) < 0)
    {
        printft("Failed to write batt mode (peak) to config register: %s\n", modbus_strerror(errno));
        reinit();
    }
    
    //Enable the inverter's utility charging time limits to prevent unwanted charging.
    DisallowOffPeakCharging();
}

static void SwitchToOffPeak()
{
    status.nSystemState = SYSTEM_STATE_OFF_PEAK;
    slModeWriteTime = time(NULL);
    
    if(modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID) < 0)
    {
        printft("Failed to write grid mode (off peak) to config register: %s\n", modbus_strerror(errno));
        reinit();
    }
    
    //Disable the inverter's utility charging time limits to take advantage of the full off-peak.
    AllowOffPeakCharging();
}

static void SwitchToBoost()
{
    status.nSystemState = SYSTEM_STATE_BOOST;
    slModeWriteTime = time(NULL);
    
    if(modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID) < 0)
    {
        printft("Failed to write grid mode (boost) to config register: %s\n", modbus_strerror(errno));
        reinit();
    }
    
    //Disable the inverter's utility charging time limits to allow any time charging.
    AllowOffPeakCharging();
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
                    printft("Could not create MODBUS context.\n");
                    reinit();
                }
                else
                {
                    //Connect to the MODBUS slave (slave ID 1)
                    sleep(1);
                    modbus_set_slave(ctx, 1);
                    if (modbus_connect(ctx) == -1)
                    {
                        printft("Could not connect MODBUS slave.\n");
                        reinit();
                    }
                    else
                    {
                        printft("MODBUS initialised. Going to processing.\n");
                    }
                }
                
                sleep(1);
            }
            break;

            case PROCESS:
            {
                if(bPrintConfigRegisters)
                {
                    bPrintConfigRegisters = false;
                    
                    printf("Reading config registers...\n");
                    
                    uint16_t configRegs[10];
                    
                    for(int i = 0; i < 36; i++)
                    {
                        memset(configRegs, 0, sizeof(configRegs));
                    
                        if(-1 == modbus_read_registers(ctx, i * 10, 10, &configRegs[0]))
                        {
                            printf("Failed to read config registers: %s\n", modbus_strerror(errno));
                            break;
                        }
                        else
                        {
                            printf("%5d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
                                   i * 10,
                                   configRegs[0],
                                   configRegs[1],
                                   configRegs[2],
                                   configRegs[3],
                                   configRegs[4],
                                   configRegs[5],
                                   configRegs[6],
                                   configRegs[7],
                                   configRegs[8],
                                   configRegs[9]);
                        }
                    }
                    
                    printf("--------------------------------\n");
                }
            
                //Read input registers and holding register (inverter mode).
                if(-1 == modbus_read_input_registers(ctx, 0, INPUT_REGISTER_COUNT, inputRegs) ||
                   -1 == modbus_read_registers(ctx, GW_HREG_CFG_MODE, 1, &nInverterMode))
                {
                    printft("Failed to read input registers and config mode via MODBUS.\n");
                    reinit();
                }
                else
                {                
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
                    if(SYSTEM_STATE_PEAK == status.nSystemState && bManualSwitchToGrid)
                    {
                        bManualSwitchToGrid = false;
                        SwitchToBypass();
                        printft("Switched to grid due to override.\n");
                    }
                    
                    //Peak/off-peak switching.
                    if ((lHour > SYSTEM_END_OFF_PEAK_H ||
                         (lHour == SYSTEM_END_OFF_PEAK_H && lMin >= SYSTEM_END_OFF_PEAK_M)) &&
                        (lHour < SYSTEM_START_OFF_PEAK_H ||
                         (lHour == SYSTEM_START_OFF_PEAK_H && lMin <= SYSTEM_START_OFF_PEAK_M)))
                    {
                        //Switch to peak if off-peak?
                        if(SYSTEM_STATE_OFF_PEAK == status.nSystemState)
                        {
                            SwitchToPeak();
                            printft("Switched to peak.\n");
                        }
                    }
                    else
                    {
                        //Switch to off-peak if peak/bypassed?
                        if(SYSTEM_STATE_OFF_PEAK != status.nSystemState)
                        {
                            SwitchToOffPeak();
                            printft("Switched to off-peak.\n");
                        }
                    }
                    
                    //Manual batts switching.
                    if(SYSTEM_STATE_BYPASS == status.nSystemState && bManualSwitchToBatts)
                    {
                        bManualSwitchToBatts = false;
                        SwitchToPeak();
                        printft("Switched to batts due to override.\n");
                    }
                    
                    //Final state check. (Note: inverter mode is read back from inverter on next pass).
                    if(time(NULL) > slModeWriteTime + CHECK_MODE_TIMEOUT)
                    {
                        switch(status.nSystemState)
                        {
                            case SYSTEM_STATE_PEAK:
                            {
                                if(GW_CFG_MODE_BATTS != nInverterMode)
                                {
                                    SwitchToPeak();
                                    printft("Wasn't on batts as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                            
                            case SYSTEM_STATE_BYPASS:
                            {
                                if(GW_CFG_MODE_GRID != nInverterMode)
                                {
                                    SwitchToBypass();
                                    printft("Wasn't on grid as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                            
                            case SYSTEM_STATE_OFF_PEAK:
                            {
                                if(GW_CFG_MODE_GRID != nInverterMode)
                                {
                                    SwitchToOffPeak();
                                    printft("Wasn't on grid as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                        }
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
                            case SYSTEM_STATE_PEAK: printf("PEAK\n"); break;
                            case SYSTEM_STATE_BYPASS: printf("BYPASS\n"); break;
                            case SYSTEM_STATE_OFF_PEAK: printf("OFF-PEAK\n"); break;
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
        
        usleep(100000);
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
                            case SYSTEM_STATE_PEAK: printf("nSystemState\tPEAK\n"); break;
                            case SYSTEM_STATE_BYPASS: printf("nSystemState\tBYPASS\n"); break;
                            case SYSTEM_STATE_OFF_PEAK: printf("nSystemState\tOFF-PEAK\n"); break;
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
                    
                    case 'c':
                    {
                        bPrintConfigRegisters = true;
                    }
                    break;
                    
                    case '\n':
                    case '\r':
                        //Ignore whitespace.
                    break;

                    default:
                    {
                        printf("---=== %c unknown - available Commands ===---\n", input);
                        printf("q - Quit\n");
                        printf("s - Print latest status\n");
                        printf("g - Manual grid mode\n");
                        printf("b - Manual battery mode\n");
                        printf("c - Read & print config registers\n");
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





























