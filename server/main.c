
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

#include "utils.h"
#include "tcpserver.h"

bool bRunning = true;
bool bPrintConfigRegisters = false;
bool bLogging = true;

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
uint16_t nInverterMode[INVERTER_COUNT];
uint16_t nChargeAmps[INVERTER_COUNT];
uint16_t nEndHour[INVERTER_COUNT];
bool bManualSwitchToGrid = false;
bool bManualSwitchToBatts = false;
bool bManualSwitchToBoost = false;
int32_t slModeWriteTime;

static int lLoggingLastMin[INVERTER_COUNT];
                            

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

    // Print the timestamp and the formatted message to the console
    printf("[%s] ", timestamp);
    vprintf(format, args);

    // Get the user's home directory
    const char* homeDir = getenv("HOME");
    if (homeDir == NULL)
    {
        // Fallback to getpwuid if HOME is not set
        struct passwd* pw = getpwuid(geteuid());
        homeDir = pw ? pw->pw_dir : ".";
    }

    // Create the 'invlogs/' directory in the user's home directory if it doesn't exist
    char logDir[256];
    snprintf(logDir, sizeof(logDir), "%s/invlogs", homeDir);
    struct stat st = {0};
    if (stat(logDir, &st) == -1)
    {
        if (mkdir(logDir, 0755) == -1) {
            printf("Error: Failed to create directory '%s': %s\n", logDir, strerror(errno));
            va_end(args);
            return;
        }
    }

    // Construct the full file path
    char filepath[256];
    if (snprintf(filepath, sizeof(filepath), "%s/%s", logDir, filename) >= sizeof(filepath))
    {
        printf("Error: File path is too long\n");
        va_end(args);
        return;
    }

    // Open the file for appending, creating it if necessary
    FILE* file = fopen(filepath, "a");
    if (file == NULL)
    {
        printf("Error: Failed to open file '%s': %s\n", filepath, strerror(errno));
        va_end(args);
        return;
    }
    
    // Print the timestamp to the file
    fprintf(file, "[%s] ", timestamp);
    
    // Write the formatted message to the file
    va_list args_copy;
    va_copy(args_copy, args);  // Create a copy of args to reuse
    vfprintf(file, format, args_copy);
    va_end(args_copy);

    // Close the file
    fclose(file);

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
    //Intelligent charging calculation.
    status.nChargeCurrent = utils_GetOffpeakChargingAmps(&status);
    
    for(int i = 0; i < INVERTER_COUNT; i++)
    {
        modbus_set_slave(ctx, i + INVERTER_1_ID);
        
        if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, status.nChargeCurrent) < 0)
        {
            printft("Failed to write steady util charging amps to config register: %s\n", modbus_strerror(errno));
            reinit();
        }
        
        usleep(100000);
    }
}

static void SetBoostAmps()
{
    status.nChargeCurrent = GW_CFG_UTIL_AMPS_MAX;
    
    for(int i = 0; i < INVERTER_COUNT; i++)
    {
        modbus_set_slave(ctx, i + INVERTER_1_ID);
        
        if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, status.nChargeCurrent) < 0)
        {
            printft("Failed to write max util charging amps to config register: %s\n", modbus_strerror(errno));
            reinit();
        }
        
        usleep(100000);
    }
}

static void SetManualAmps(uint16_t nAmps)
{
    if(nAmps <= GW_HREG_MAX_UTIL_AMPS)
    {
        status.nChargeCurrent = nAmps;

        for(int i = 0; i < INVERTER_COUNT; i++)
        {
            modbus_set_slave(ctx, i + INVERTER_1_ID);
            
            if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, status.nChargeCurrent) < 0)
            {
                printft("Failed to write manual util charging amps to config register: %s\n", modbus_strerror(errno));
                reinit();
            }
            
            usleep(100000);
        }
    }
}

static void LimitChargingTimes()
{
    for(int i = 0; i < INVERTER_COUNT; i++)
    {
        modbus_set_slave(ctx, i + INVERTER_1_ID);

        if(modbus_write_register(ctx, GW_HREG_UTIL_END_HOUR, GW_CFG_UTIL_TIME_OFFPEAK) < 0)
        {
            printft("Failed to write off-peak only charging config register: %s\n", modbus_strerror(errno));
            reinit();
        }
        
        usleep(100000);
    }
}

static void UnlimitChargingTimes()
{
    for(int i = 0; i < INVERTER_COUNT; i++)
    {
        modbus_set_slave(ctx, i + INVERTER_1_ID);

        if(modbus_write_register(ctx, GW_HREG_UTIL_END_HOUR, GW_CFG_UTIL_TIME_ANY_TIME) < 0)
        {
            printft("Failed to write any time charging config register: %s\n", modbus_strerror(errno));
            reinit();
        }
        
        usleep(100000);
    }
}

static void SwitchToBypass()
{
    status.nSystemState = SYSTEM_STATE_BYPASS;
    slModeWriteTime = time(NULL);
    
    for(int i = 0; i < INVERTER_COUNT; i++)
    {
        modbus_set_slave(ctx, i + INVERTER_1_ID);

        if(modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID) < 0)
        {
            printft("Failed to write grid mode (bypass) to config register: %s\n", modbus_strerror(errno));
            reinit();
        }
        
        usleep(100000);
    }
    
    //Enable the inverter's utility charging time limits to prevent unwanted charging.
    LimitChargingTimes();
}

static void SwitchToPeak()
{
    status.nSystemState = SYSTEM_STATE_PEAK;
    slModeWriteTime = time(NULL);
    
    for(int i = 0; i < INVERTER_COUNT; i++)
    {
        modbus_set_slave(ctx, i + INVERTER_1_ID);
        
        if(modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_BATTS) < 0)
        {
            printft("Failed to write batt mode (peak) to config register: %s\n", modbus_strerror(errno));
            reinit();
        }
        
        usleep(100000);
    }
    
    //Enable the inverter's utility charging time limits to prevent unwanted charging.
    LimitChargingTimes();
}

static void SwitchToOffPeak()
{
    status.nSystemState = SYSTEM_STATE_OFF_PEAK;
    slModeWriteTime = time(NULL);
    
    for(int i = 0; i < INVERTER_COUNT; i++)
    {
        modbus_set_slave(ctx, i + INVERTER_1_ID);
        
        if(modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID) < 0)
        {
            printft("Failed to write grid mode (off peak) to config register: %s\n", modbus_strerror(errno));
            reinit();
        }
        
        usleep(100000);
    }
    
    //Disable the inverter's utility charging time limits to take advantage of the full off-peak.
    UnlimitChargingTimes();
    
    //Set overnight (load balancing) amps.
    SetOvernightAmps();
}

static void SwitchToBoost()
{
    status.nSystemState = SYSTEM_STATE_BOOST;
    slModeWriteTime = time(NULL);
    
    for(int i = 0; i < INVERTER_COUNT; i++)
    {
        modbus_set_slave(ctx, i + INVERTER_1_ID);
        
        if(modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID) < 0)
        {
            printft("Failed to write grid mode (boost) to config register: %s\n", modbus_strerror(errno));
            reinit();
        }
        
        usleep(100000);
    }
    
    //Disable the inverter's utility charging time limits to allow any time charging.
    UnlimitChargingTimes();
    
    //Set boost amps.
    SetBoostAmps();
}

void* modbus_thread(void* arg)
{
    for(int i = 0; i < INVERTER_COUNT; i++)
    {
        lLoggingLastMin[i] = -1;
    }

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
                    
                    for(int i = 0; i < INVERTER_COUNT; i++)
                    {
                        printf("Inverter %d:\n", i + INVERTER_1_ID);
                        
                        uint16_t configRegs[10];
                        
                        for(int i = 0; i < 36; i++)
                        {
                            memset(configRegs, 0, sizeof(configRegs));
                            
                            modbus_set_slave(ctx, i + INVERTER_1_ID);
                        
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
                }
                
                //Get the time.
                time_t rawtime;
                struct tm* timeinfo;
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                int lHour = timeinfo->tm_hour;
                int lMin = timeinfo->tm_min;
            
                //Read input registers and holding register (inverter mode).
                for(int i = 0; i < INVERTER_COUNT; i++)
                {
                    uint16_t inputRegs[INPUT_REGISTER_COUNT];

                    modbus_set_slave(ctx, i + INVERTER_1_ID);
                    
                    int inputRegRead = modbus_read_input_registers(ctx, 0, INPUT_REGISTER_COUNT, inputRegs);
                    usleep(10000);
                    int holdingRegRead1 = modbus_read_registers(ctx, GW_HREG_CFG_MODE, 1, &nInverterMode[i]);
                    usleep(10000);
                    int holdingRegRead2 = modbus_read_registers(ctx, GW_HREG_MAX_UTIL_AMPS, 1, &nChargeAmps[i]);
                    usleep(10000);
                    int holdingRegRead3 = modbus_read_registers(ctx, GW_HREG_UTIL_END_HOUR, 1, &nEndHour[i]);
                    usleep(10000);
                    
                    if(-1 == inputRegRead ||
                       -1 == holdingRegRead1 ||
                       -1 == holdingRegRead2 ||
                       -1 == holdingRegRead3 )
                    {
                        printft("Failed to read MODBUS registers for inverter %d.\n", i + INVERTER_1_ID);
                        reinit();
                        break;
                    }
                    else
                    {
                        //Store relevant input register values.
                        status.nInverterState[i] = inputRegs[STATUS];
                        status.nOutputWatts[i] = inputRegs[OUTPUT_WATTS_L];
                        status.nOutputApppwr[i] = inputRegs[OUTPUT_APPPWR_L];
                        status.nAcChargeWattsL[i] = inputRegs[AC_CHARGE_WATTS_L];
                        status.nBatteryVolts[i] = inputRegs[BATTERY_VOLTS];
                        status.nBusVolts[i] = inputRegs[BUS_VOLTS];
                        status.nGridVolts[i] = inputRegs[GRID_VOLTS];
                        status.nGridFreq[i] = inputRegs[GRID_FREQ];
                        status.nAcOutVolts[i] = inputRegs[AC_OUT_VOLTS];
                        status.nAcOutFreq[i] = inputRegs[AC_OUT_FREQ];
                        status.nInverterTemp[i] = inputRegs[INVERTER_TEMP];
                        status.nDCDCTemp[i] = inputRegs[DCDC_TEMP];
                        status.nLoadPercent[i] = inputRegs[LOAD_PERCENT];
                        status.nBuck1Temp[i] = inputRegs[BUCK1_TEMP];
                        status.nBuck2Temp[i] = inputRegs[BUCK2_TEMP];
                        status.nOutputAmps[i] = inputRegs[OUTPUT_AMPS];
                        status.nInverterAmps[i] = inputRegs[INVERTER_AMPS];
                        status.nAcInputWattsL[i] = inputRegs[AC_INPUT_WATTS_L];
                        status.nAcchgegyToday[i] = inputRegs[ACCHGEGY_TODAY_L];
                        status.nBattuseToday[i] = inputRegs[BATTUSE_TODAY_L];
                        status.nAcUseToday[i] = inputRegs[AC_USE_TODAY_L];
                        status.nBattchgAmps[i] = inputRegs[BATTCHG_AMPS];
                        status.nAcUseWatts[i] = inputRegs[AC_USE_WATTS_L];
                        status.nBattUseWatts[i] = inputRegs[BATTUSE_WATTS_L];
                        status.nBattWatts[i] = inputRegs[BATT_WATTS_L];
                        status.nInvFanspeed[i] = inputRegs[INV_FANSPEED];
                        
                        if(bLogging)
                        {
                            if (lMin % 5 == 0 && lMin != lLoggingLastMin[i])
                            {
                                //Log all values to files every five minutes.
                                printftlog("InverterState", "%d\n", status.nInverterState[i]);
                                printftlog("OutputWatts", "%d\n", status.nOutputWatts[i]);
                                printftlog("OutputApppwr", "%d\n", status.nOutputApppwr[i]);
                                printftlog("AcChargeWattsL", "%d\n", status.nAcChargeWattsL[i]);
                                printftlog("BatteryVolts", "%d\n", status.nBatteryVolts[i]);
                                printftlog("BusVolts", "%d\n", status.nBusVolts[i]);
                                printftlog("GridVolts", "%d\n", status.nGridVolts[i]);
                                printftlog("GridFreq", "%d\n", status.nGridFreq[i]);
                                printftlog("AcOutVolts", "%d\n", status.nAcOutVolts[i]);
                                printftlog("AcOutFreq", "%d\n", status.nAcOutFreq[i]);
                                printftlog("InverterTemp", "%d\n", status.nInverterTemp[i]);
                                printftlog("DCDCTemp", "%d\n", status.nDCDCTemp[i]);
                                printftlog("LoadPercent", "%d\n", status.nLoadPercent[i]);
                                printftlog("Buck1Temp", "%d\n", status.nBuck1Temp[i]);
                                printftlog("Buck2Temp", "%d\n", status.nBuck2Temp[i]);
                                printftlog("OutputAmps", "%d\n", status.nOutputAmps[i]);
                                printftlog("InverterAmps", "%d\n", status.nInverterAmps[i]);
                                printftlog("AcInputWattsL", "%d\n", status.nAcInputWattsL[i]);
                                printftlog("AcchgegyToday", "%d\n", status.nAcchgegyToday[i]);
                                printftlog("BattuseToday", "%d\n", status.nBattuseToday[i]);
                                printftlog("AcUseToday", "%d\n", status.nAcUseToday[i]);
                                printftlog("BattchgAmps", "%d\n", status.nBattchgAmps[i]);
                                printftlog("AcUseWatts", "%d\n", status.nAcUseWatts[i]);
                                printftlog("BattuseWatts", "%d\n", status.nBattUseWatts[i]);
                                printftlog("BattWatts", "%d\n", status.nBattWatts[i]);
                                printftlog("InvFanspeed", "%d\n", status.nInvFanspeed[i]);
                            
                                lLoggingLastMin[i] = lMin;
                            }
                        }
                    }
                }
                
                //Do general processing if reading the inverters went okay, using the values read from the master inverter.
                if(PROCESS == modbusState)
                {
                    //Find and record the off-peak charge completion time.
                    if(SYSTEM_STATE_OFF_PEAK == status.nSystemState)
                    {
                        if(status.nBattchgAmps > 0 && status.nBattchgAmps[0] == 0)
                        {
                            status.slOffPeakChgComplete = time(NULL);
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
                            //Store the morning's AC charge energy so any boost charging can be accounted for later.
                            status.nOffPeakChargeKwh = 0;
                            
                            for(int i = 0; i < INVERTER_COUNT; i++)
                            {
                                status.nOffPeakChargeKwh += status.nAcchgegyToday[i];
                            }
                        
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
                                if(GW_CFG_MODE_BATTS != nInverterMode[0])
                                {
                                    SwitchToPeak();
                                    printft("Wasn't on batts as expected. Rewrote holding register.\n");
                                }
                                
                                if(GW_CFG_UTIL_TIME_OFFPEAK != nEndHour[0])
                                {
                                    LimitChargingTimes();
                                    printft("Charging times not limited as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                            
                            case SYSTEM_STATE_BYPASS:
                            {
                                if(GW_CFG_MODE_GRID != nInverterMode[0])
                                {
                                    SwitchToBypass();
                                    printft("Wasn't on grid as expected. Rewrote holding register.\n");
                                }
                                
                                if(GW_CFG_UTIL_TIME_OFFPEAK != nEndHour[0])
                                {
                                    LimitChargingTimes();
                                    printft("Charging times not limited as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                            
                            case SYSTEM_STATE_OFF_PEAK:
                            {
                                if(GW_CFG_MODE_GRID != nInverterMode[0])
                                {
                                    SwitchToOffPeak();
                                    printft("Wasn't on grid as expected. Rewrote holding register.\n");
                                }
                                
                                if(GW_CFG_UTIL_TIME_ANY_TIME != nEndHour[0])
                                {
                                    UnlimitChargingTimes();
                                    printft("Charging times not unlimited as expected. Rewrote holding register.\n");
                                }
                                
                                if(status.nChargeCurrent != nChargeAmps[0])
                                {
                                    SetOvernightAmps();
                                    printft("Charging amps not set as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                            
                            case SYSTEM_STATE_BOOST:
                            {
                                if(GW_CFG_MODE_GRID != nInverterMode[0])
                                {
                                    SwitchToBoost();
                                    printft("Wasn't on boost as expected. Rewrote holding register.\n");
                                }
                                
                                if(GW_CFG_UTIL_TIME_ANY_TIME != nEndHour[0])
                                {
                                    UnlimitChargingTimes();
                                    printft("Charging times not unlimited as expected. Rewrote holding register.\n");
                                }
                                
                                if(status.nChargeCurrent != nChargeAmps[0])
                                {
                                    SetBoostAmps();
                                    printft("Charging amps not as expected. Rewrote holding register.\n");
                                }
                            }
                            break;
                        }
                    }
                    
                    //Print state changes.
                    if(nLastInverterMode != nInverterMode[0])
                    {
                        printft("nInverterMode changed to ");
                        switch(nInverterMode[0])
                        {
                            case GW_CFG_MODE_BATTS: printf("BATTERIES\n"); break;
                            case GW_CFG_MODE_GRID: printf("GRID\n"); break;
                            default: printf("GOD KNOWS! (%d)\n", nInverterMode[0]); break;
                        }
                
                        nLastInverterMode = nInverterMode[0];
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
                    
                    if(nLastInverterState != status.nInverterState[0])
                    {
                        printft("nInverterState changed to ");
                        if(status.nInverterState[0] >= 0 && status.nInverterState[0] < INVERTER_STATE_COUNT)
                            printf("%s\n", GwInverterStatusStrings[status.nInverterState[0]]);
                        else
                            printf("UNKNOWN (%d)\n", status.nInverterState[0]);
                            
                        nLastInverterState = status.nInverterState[0];
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
                        for(int i = 0; i < INVERTER_COUNT; i++)
                        {
                            printf("---=== Status ===---\n");
                            printf("Inverter %d:\n", i + INVERTER_1_ID);
                            switch(nInverterMode[i])
                            {
                                case GW_CFG_MODE_BATTS: printf("nInverterMode\tBATTERIES\n"); break;
                                case GW_CFG_MODE_GRID: printf("nInverterMode\tGRID\n"); break;
                                default: printf("nInverterMode\tGOD KNOWS! (%d)\n", nInverterMode[i]); break;
                            }
                            
                            switch(status.nSystemState)
                            {
                                case SYSTEM_STATE_PEAK: printf("nSystemState\tPEAK\n"); break;
                                case SYSTEM_STATE_BYPASS: printf("nSystemState\tBYPASS\n"); break;
                                case SYSTEM_STATE_OFF_PEAK: printf("nSystemState\tOFF-PEAK\n"); break;
                                case SYSTEM_STATE_BOOST: printf("nSystemState\tBOOST\n"); break;
                                default: printf("nSystemState\tGOD KNOWS! (%d)\n", status.nSystemState); break;
                            }
                            
                            if(status.nInverterState[i] >= 0 && status.nInverterState[i] < INVERTER_STATE_COUNT)
                                printf("nInverterState\t%s\n", GwInverterStatusStrings[status.nInverterState[i]]);
                            else
                                printf("nInverterState\t UNKNOWN (%d)\n", status.nInverterState[i]);
                            
                            printf("\n");
                            printf("slModeWriteTime\t%d\n", slModeWriteTime);
                            printf("The actual time\t%ld\n", time(NULL));
                        
                            printf("\n");
                            printf("nOutputWatts\t%d\n", status.nOutputWatts[i]);
                            printf("nOutputApppwr\t%d\n", status.nOutputApppwr[i]);
                            printf("nAcChargeWattsL\t%d\n", status.nAcChargeWattsL[i]);
                            printf("nBatteryVolts\t%d\n", status.nBatteryVolts[i]);
                            printf("nBusVolts\t%d\n", status.nBusVolts[i]);
                            printf("nGridVolts\t%d\n", status.nGridVolts[i]);
                            printf("nGridFreq\t%d\n", status.nGridFreq[i]);
                            printf("nAcOutVolts\t%d\n", status.nAcOutVolts[i]);
                            printf("nAcOutFreq\t%d\n", status.nAcOutFreq[i]);
                            printf("nInverterTemp\t%d\n", status.nInverterTemp[i]);
                            printf("nDCDCTemp\t%d\n", status.nDCDCTemp[i]);
                            printf("nLoadPercent\t%d\n", status.nLoadPercent[i]);
                            printf("nBuck1Temp\t%d\n", status.nBuck1Temp[i]);
                            printf("nBuck2Temp\t%d\n", status.nBuck2Temp[i]);
                            printf("nOutputAmps\t%d\n", status.nOutputAmps[i]);
                            printf("nInverterAmps\t%d\n", status.nInverterAmps[i]);
                            printf("nAcInputWattsL\t%d\n", status.nAcInputWattsL[i]);
                            printf("nAcchgegyToday\t%d\n", status.nAcchgegyToday[i]);
                            printf("nBattuseToday\t%d\n", status.nBattuseToday[i]);
                            printf("nAcUseToday\t%d\n", status.nAcUseToday[i]);
                            printf("nBattchgAmps\t%d\n", status.nBattchgAmps[i]);
                            printf("nAcUseWatts\t%d\n", status.nAcUseWatts[i]);
                            printf("nBattuseWatts\t%d\n", status.nBattUseWatts[i]);
                            printf("nBattWatts\t%d\n", status.nBattWatts[i]);
                            printf("nInvFanspeed\t%d\n", status.nInvFanspeed[i]);
                            printf("--------------------\n");
                        }
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
                    
                    case 'f':
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
                    
                    case 'a':
                    {
                        uint16_t nAmps = 0;
                        
                        if (scanf("%hd", &nAmps) == 1 && nAmps >= 1 && nAmps <= 80)
                        {
                            printf("Setting charge override to %d amps\n", nAmps);
                            SetManualAmps(nAmps);
                            
                            if(status.nSystemState == SYSTEM_STATE_PEAK ||
                               status.nSystemState == SYSTEM_STATE_BYPASS)
                            {
                                printf("Note: This only applies during charging! State changes will override it again!\n");
                            }
                        }
                        else
                        {
                            printf("Invalid input. Please enter a number between 1 and 80.\n");
                            // Clear the rest of the input buffer in case of invalid input
                            while ((input = getchar()) != '\n' && input != EOF);
                        }
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
                        printf("f - Manual boost mode\n");
                        printf("c - Read & print config registers\n");
                        printf("l - Toggle logging\n");
                        printf("d - Toggle more debug\n");
                        printf("a[1-80] - Override current util charge amps\n");
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





























