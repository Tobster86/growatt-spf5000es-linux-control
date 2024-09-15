
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
#include <sys/stat.h>
#include <pwd.h>

#include <modbus.h>
#include <errno.h>

#include "spf5000es_defs.h"
#include "system_defs.h"
#include "tcpserver.h"

bool bRunning = true;
bool bPrintConfigRegisters = false;
bool bLogging = false;
bool bDebug = false;

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
uint16_t nChargeAmps;
uint16_t nEndHour;
bool bManualSwitchToGrid = false;
bool bManualSwitchToBatts = false;
bool bManualSwitchToBoost = false;
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
    bManualSwitchToBoost = false;
}

void _tcpserver_SetGrid()
{
    printft("Remote user requested switch to grid.\n");
    bManualSwitchToGrid = true;
    bManualSwitchToBatts = false;
    bManualSwitchToBoost = false;
}

void _tcpserver_SetBoost()
{
    printft("Remote user requested switch to boost.\n");
    bManualSwitchToGrid = false;
    bManualSwitchToBatts = false;
    bManualSwitchToBoost = true;
}

//Local functions.
//Printf with timestamp.
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

//Printf with timestamp and log to file.
static void printftlog(const char* filename, const char* format, ...)
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

    // Get the user's home directory
    const char* homeDir = getenv("HOME");
    if (homeDir == NULL) {
        // Fallback to getpwuid if HOME is not set
        struct passwd* pw = getpwuid(geteuid());
        homeDir = pw ? pw->pw_dir : ".";
    }

    // Create the 'invlogs/' directory in the user's home directory if it doesn't exist
    char logDir[256];
    snprintf(logDir, sizeof(logDir), "%s/invlogs", homeDir);
    struct stat st = {0};
    if (stat(logDir, &st) == -1) {
        mkdir(logDir, 0755);  // Create the 'invlogs/' directory with read/write/execute permissions
    }

    // Construct the full file path
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", logDir, filename);

    // Open the file for appending, creating it if necessary
    FILE* file = fopen(filepath, "a");
    if (file != NULL)
    {
        // Print the timestamp to the file
        fprintf(file, "[%s] ", timestamp);
        
        // Write the formatted message to the file
        va_list args_copy;
        va_copy(args_copy, args);  // Create a copy of args to reuse
        vfprintf(file, format, args_copy);
        va_end(args_copy);

        // Close the file
        fclose(file);
    }

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
    //TO DO: Smart amps calculation will happen here.

    if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, GW_CFG_UTIL_AMPS_MOD) < 0)
    {
        printft("Failed to write steady util charging amps to config register: %s\n", modbus_strerror(errno));
        reinit();
    }
    
    usleep(100000);
}

static void SetBoostAmps()
{
    if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, GW_CFG_UTIL_AMPS_MAX) < 0)
    {
        printft("Failed to write max util charging amps to config register: %s\n", modbus_strerror(errno));
        reinit();
    }
    
    usleep(100000);
}

static void LimitChargingTimes()
{
    if(modbus_write_register(ctx, GW_HREG_UTIL_END_HOUR, GW_CFG_UTIL_TIME_OFFPEAK) < 0)
    {
        printft("Failed to write off-peak only charging config register: %s\n", modbus_strerror(errno));
        reinit();
    }
    
    usleep(100000);
}

static void UnlimitChargingTimes()
{
    if(modbus_write_register(ctx, GW_HREG_UTIL_END_HOUR, GW_CFG_UTIL_TIME_ANY_TIME) < 0)
    {
        printft("Failed to write any time charging config register: %s\n", modbus_strerror(errno));
        reinit();
    }
    
    usleep(100000);
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
    
    usleep(100000);
    
    //Enable the inverter's utility charging time limits to prevent unwanted charging.
    LimitChargingTimes();
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
    
    usleep(100000);
    
    //Enable the inverter's utility charging time limits to prevent unwanted charging.
    LimitChargingTimes();
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
    
    usleep(100000);
    
    //Disable the inverter's utility charging time limits to take advantage of the full off-peak.
    UnlimitChargingTimes();
    
    //Set overnight (load balancing) amps.
    SetOvernightAmps();
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
    
    usleep(100000);
    
    //Disable the inverter's utility charging time limits to allow any time charging.
    UnlimitChargingTimes();
    
    //Set boost amps.
    SetBoostAmps();
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
                int inputRegRead = modbus_read_input_registers(ctx, 0, INPUT_REGISTER_COUNT, inputRegs);
                usleep(10000);
                int holdingRegRead1 = modbus_read_registers(ctx, GW_HREG_CFG_MODE, 1, &nInverterMode);
                usleep(10000);
                int holdingRegRead2 = modbus_read_registers(ctx, GW_HREG_MAX_UTIL_AMPS, 1, &nChargeAmps);
                usleep(10000);
                int holdingRegRead3 = modbus_read_registers(ctx, GW_HREG_UTIL_END_HOUR, 1, &nEndHour);
                usleep(10000);
                
                if(-1 == inputRegRead ||
                   -1 == holdingRegRead1 ||
                   -1 == holdingRegRead2 ||
                   -1 == holdingRegRead3 )
                {
                    printft("Failed to read input registers and config mode via MODBUS.\n");
                    reinit();
                }
                else
                {
                    if(bDebug)
                    {
                        printft("nInverterMode =\t%d.\n");
                        printft("nChargeAmps =\t%d.\n");
                        printft("nEndHour =\t%d.\n");
                    }
                
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
                    status.nAcChargeWattsL = inputRegs[AC_CHARGE_WATTS_L];
                    status.nBatteryVolts = inputRegs[BATTERY_VOLTS];
                    status.nBusVolts = inputRegs[BUS_VOLTS];
                    status.nGridVolts = inputRegs[GRID_VOLTS];
                    status.nGridFreq = inputRegs[GRID_FREQ];
                    status.nAcOutVolts = inputRegs[AC_OUT_VOLTS];
                    status.nAcOutFreq = inputRegs[AC_OUT_FREQ];
                    status.nInverterTemp = inputRegs[INVERTER_TEMP];
                    status.nDCDCTemp = inputRegs[DCDC_TEMP];
                    status.nLoadPercent = inputRegs[LOAD_PERCENT];
                    status.nBuck1Temp = inputRegs[BUCK1_TEMP];
                    status.nBuck2Temp = inputRegs[BUCK2_TEMP];
                    status.nOutputAmps = inputRegs[OUTPUT_AMPS];
                    status.nInverterAmps = inputRegs[INVERTER_AMPS];
                    status.nAcInputWattsL = inputRegs[AC_INPUT_WATTS_L];
                    status.nAcchgegyToday = inputRegs[ACCHGEGY_TODAY_L];
                    status.nBattuseToday = inputRegs[BATTUSE_TODAY_L];
                    status.nAcUseToday = inputRegs[AC_USE_TODAY_L];
                    status.nAcBattchgAmps = inputRegs[AC_BATTCHG_AMPS];
                    status.nAcUseWatts = inputRegs[AC_USE_WATTS_L];
                    status.nBattuseWatts = inputRegs[BATTUSE_WATTS_L];
                    status.nBattWatts = inputRegs[BATT_WATTS_L];
                    status.nInvFanspeed = inputRegs[INV_FANSPEED];
                    
                    if(bLogging)
                    {
                        static int lLastMin = -1;
                        
                        if (lMin % 5 == 0 && lMin != lLastMin)
                        {
                            //Log all values to files every five minutes.
                            printftlog("InverterState", "%d\n", status.nInverterState);
                            printftlog("OutputWatts", "%d\n", status.nOutputWatts);
                            printftlog("OutputApppwr", "%d\n", status.nOutputApppwr);
                            printftlog("AcChargeWattsL", "%d\n", status.nAcChargeWattsL);
                            printftlog("BatteryVolts", "%d\n", status.nBatteryVolts);
                            printftlog("BusVolts", "%d\n", status.nBusVolts);
                            printftlog("GridVolts", "%d\n", status.nGridVolts);
                            printftlog("GridFreq", "%d\n", status.nGridFreq);
                            printftlog("AcOutVolts", "%d\n", status.nAcOutVolts);
                            printftlog("AcOutFreq", "%d\n", status.nAcOutFreq);
                            printftlog("InverterTemp", "%d\n", status.nInverterTemp);
                            printftlog("DCDCTemp", "%d\n", status.nDCDCTemp);
                            printftlog("LoadPercent", "%d\n", status.nLoadPercent);
                            printftlog("Buck1Temp", "%d\n", status.nBuck1Temp);
                            printftlog("Buck2Temp", "%d\n", status.nBuck2Temp);
                            printftlog("OutputAmps", "%d\n", status.nOutputAmps);
                            printftlog("InverterAmps", "%d\n", status.nInverterAmps);
                            printftlog("AcInputWattsL", "%d\n", status.nAcInputWattsL);
                            printftlog("AcchgegyToday", "%d\n", status.nAcchgegyToday);
                            printftlog("BattuseToday", "%d\n", status.nBattuseToday);
                            printftlog("AcUseToday", "%d\n", status.nAcUseToday);
                            printftlog("AcBattchgAmps", "%d\n", status.nAcBattchgAmps);
                            printftlog("AcUseWatts", "%d\n", status.nAcUseWatts);
                            printftlog("BattuseWatts", "%d\n", status.nBattuseWatts);
                            printftlog("BattWatts", "%d\n", status.nBattWatts);
                            printftlog("InvFanspeed", "%d\n", status.nInvFanspeed);
                        
                            lLastMin = lMin;
                        }
                    }
                
                    //Manual switching.
                    if(SYSTEM_STATE_OFF_PEAK != status.nSystemState)
                    {
                        //Grid.
                        if(bManualSwitchToGrid)
                        {
                            bManualSwitchToGrid = false;
                            SwitchToBypass();
                            printft("Switched to grid due to override.\n");
                        }
                        
                        //Batts.
                        if(bManualSwitchToBatts)
                        {
                            bManualSwitchToBatts = false;
                            SwitchToPeak();
                            printft("Switched to batts due to override.\n");
                        }
                        
                        //Boost.
                        if(bManualSwitchToBoost)
                        {
                            bManualSwitchToBoost = false;
                            SwitchToBoost();
                            printft("Switched to boost due to override.\n");
                        }
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
                                
                                if(GW_CFG_UTIL_TIME_OFFPEAK != nEndHour)
                                {
                                    LimitChargingTimes();
                                    printft("Charging times not limited as expected. Rewrote holding register.\n");
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
                                
                                if(GW_CFG_UTIL_TIME_OFFPEAK != nEndHour)
                                {
                                    LimitChargingTimes();
                                    printft("Charging times not limited as expected. Rewrote holding register.\n");
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
                                
                                if(GW_CFG_UTIL_TIME_ANY_TIME != nEndHour)
                                {
                                    UnlimitChargingTimes();
                                    printft("Charging times not unlimited as expected. Rewrote holding register.\n");
                                }
                                
                                if(GW_CFG_UTIL_AMPS_MOD != nChargeAmps)
                                {
                                    SetOvernightAmps();
                                    printft("Charging amps not moderate as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                            
                            case SYSTEM_STATE_BOOST:
                            {
                                if(GW_CFG_MODE_GRID != nInverterMode)
                                {
                                    SwitchToBoost();
                                    printft("Wasn't on boost as expected. Rewrote holding register.\n");
                                }
                                
                                if(GW_CFG_UTIL_TIME_ANY_TIME != nEndHour)
                                {
                                    UnlimitChargingTimes();
                                    printft("Charging times not unlimited as expected. Rewrote holding register.\n");
                                }
                                
                                if(GW_CFG_UTIL_AMPS_MAX != nChargeAmps)
                                {
                                    SetBoostAmps();
                                    printft("Charging amps not max as expected. Rewrote holding register.\n");
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
                            case SYSTEM_STATE_BOOST: printf("BOOST\n"); break;
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
                            case SYSTEM_STATE_BOOST: printf("nSystemState\tBOOST\n"); break;
                            default: printf("nSystemState\tGOD KNOWS! (%d)\n", status.nSystemState); break;
                        }
                        
                        if(status.nInverterState >= 0 && status.nInverterState < INVERTER_STATE_COUNT)
                            printf("nInverterState\t%s\n", GwInverterStatusStrings[status.nInverterState]);
                        else
                            printf("nInverterState\t UNKNOWN (%d)\n", status.nInverterState);
                        
                        printf("\n");
                        printf("slModeWriteTime\t%d\n", slModeWriteTime);
                        printf("The actual time\t%ld\n", time(NULL));
                        
                        printf("\n");
                        printf("nOutputWatts\t%d\n", status.nOutputWatts);
                        printf("nOutputApppwr\t%d\n", status.nOutputApppwr);
                        printf("nAcChargeWattsL\t%d\n", status.nAcChargeWattsL);
                        printf("nBatteryVolts\t%d\n", status.nBatteryVolts);
                        printf("nBusVolts\t%d\n", status.nBusVolts);
                        printf("nGridVolts\t%d\n", status.nGridVolts);
                        printf("nGridFreq\t%d\n", status.nGridFreq);
                        printf("nAcOutVolts\t%d\n", status.nAcOutVolts);
                        printf("nAcOutFreq\t%d\n", status.nAcOutFreq);
                        printf("nInverterTemp\t%d\n", status.nInverterTemp);
                        printf("nDCDCTemp\t%d\n", status.nDCDCTemp);
                        printf("nLoadPercent\t%d\n", status.nLoadPercent);
                        printf("nBuck1Temp\t%d\n", status.nBuck1Temp);
                        printf("nBuck2Temp\t%d\n", status.nBuck2Temp);
                        printf("nOutputAmps\t%d\n", status.nOutputAmps);
                        printf("nInverterAmps\t%d\n", status.nInverterAmps);
                        printf("nAcInputWattsL\t%d\n", status.nAcInputWattsL);
                        printf("nAcchgegyToday\t%d\n", status.nAcchgegyToday);
                        printf("nBattuseToday\t%d\n", status.nBattuseToday);
                        printf("nAcUseToday\t%d\n", status.nAcUseToday);
                        printf("nAcBattchgAmps\t%d\n", status.nAcBattchgAmps);
                        printf("nAcUseWatts\t%d\n", status.nAcUseWatts);
                        printf("nBattuseWatts\t%d\n", status.nBattuseWatts);
                        printf("nBattWatts\t%d\n", status.nBattWatts);
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
                    
                    case 'h':
                    {
                        printf("Forcing boost...\n");
                        bManualSwitchToBoost = true;
                    }
                    break;
                    
                    case 'c':
                    {
                        bPrintConfigRegisters = true;
                    }
                    break;
                    
                    case 'l':
                    {
                        bLogging = !bLogging;
                        printf(bLogging? "Logging\n" : "Stopped logging\n");
                    }
                    break;
                    
                    case 'd':
                    {
                        bDebug = !bDebug;
                        printf(bDebug? "More debug\n" : "Stopped more debug\n");
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
                        printf("h - Manual boost mode\n");
                        printf("c - Read & print config registers\n");
                        printf("l - toggle logging\n");
                        printf("d - toggle more debug\n");
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





























