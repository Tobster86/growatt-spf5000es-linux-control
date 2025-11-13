
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
bool bLogging = true;
bool bMODBUSDebug = false;
bool bDumpInputRegs = false;

#define MODBUS_WAIT 150000

#define NUM_INVERTERS 2

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

static int lLoggingLastMin;
                            

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
    
    if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, status.nChargeCurrent) < 0)
    {
        printft("Failed to write steady util charging amps to config register: %s\n", modbus_strerror(errno));
    }
    
    usleep(MODBUS_WAIT);
}

static void SetBoostAmps()
{
    status.nChargeCurrent = GW_CFG_UTIL_AMPS_MAX;
    
    if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, status.nChargeCurrent) < 0)
    {
        printft("Failed to write max util charging amps to config register: %s\n", modbus_strerror(errno));
    }
    
    usleep(MODBUS_WAIT);
}

static void SetManualAmps(uint16_t nAmps)
{
    if(nAmps <= GW_HREG_MAX_UTIL_AMPS)
    {
        status.nChargeCurrent = nAmps;

        if(modbus_write_register(ctx, GW_HREG_MAX_UTIL_AMPS, status.nChargeCurrent) < 0)
        {
            printft("Failed to write manual util charging amps to config register: %s\n", modbus_strerror(errno));
        }
        
        usleep(MODBUS_WAIT);
    }
}

static void LimitChargingTimes()
{
    if(modbus_write_register(ctx, GW_HREG_UTIL_END_HOUR, GW_CFG_UTIL_TIME_OFFPEAK) < 0)
    {
        printft("Failed to write off-peak only charging config register: %s\n", modbus_strerror(errno));
    }
    
    usleep(MODBUS_WAIT);
}

static void UnlimitChargingTimes()
{
    if(modbus_write_register(ctx, GW_HREG_UTIL_END_HOUR, GW_CFG_UTIL_TIME_ANY_TIME) < 0)
    {
        printft("Failed to write any time charging config register: %s\n", modbus_strerror(errno));
    }
    
    usleep(MODBUS_WAIT);
}

static void SwitchToBypass()
{
    status.nSystemState = SYSTEM_STATE_BYPASS;
    slModeWriteTime = time(NULL);
    
    if(modbus_write_register(ctx, GW_HREG_CFG_MODE, GW_CFG_MODE_GRID) < 0)
    {
        printft("Failed to write grid mode (bypass) to config register: %s\n", modbus_strerror(errno));
    }
    
    usleep(MODBUS_WAIT);
    
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
    }
    
    usleep(MODBUS_WAIT);
    
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
    }
    
    usleep(MODBUS_WAIT);
    
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
    }
    
    usleep(MODBUS_WAIT);

    //Disable the inverter's utility charging time limits to allow any time charging.
    UnlimitChargingTimes();
    
    //Set boost amps.
    SetBoostAmps();
}

void* modbus_thread(void* arg)
{
    lLoggingLastMin = -1;

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
                    //Connect to the MODBUS.
                    sleep(1);
                    if (modbus_connect(ctx) == -1)
                    {
                        printft("Could not connect MODBUS slave.\n");
                        reinit();
                    }
                    else
                    {
                        modbus_set_slave(ctx, INVERTER_1_ID);
                        printft("MODBUS initialised. Going to processing.\n");
                    }
                }
                
                sleep(1);
            }
            break;

            case PROCESS:
            {
                modbus_set_debug(ctx, bMODBUSDebug);
                
                //Get the time.
                time_t rawtime;
                struct tm* timeinfo;
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                int lHour = timeinfo->tm_hour;
                int lMin = timeinfo->tm_min;
            
                //Read input registers and holding register (inverter mode).
                uint16_t inputRegs[INPUT_REGISTER_COUNT * NUM_INVERTERS];

                for(int i = 0; i < NUM_INVERTERS; i++)
                {
                    int inputRegRead = modbus_read_input_registers(ctx, i * INPUT_REGISTER_COUNT, INPUT_REGISTER_COUNT, &inputRegs[i * INPUT_REGISTER_COUNT]);
                    usleep(MODBUS_WAIT);
                }
                
                int holdingRegRead1 = modbus_read_registers(ctx, GW_HREG_CFG_MODE, 1, &nInverterMode);
                usleep(MODBUS_WAIT);
                int holdingRegRead2 = modbus_read_registers(ctx, GW_HREG_MAX_UTIL_AMPS, 1, &nChargeAmps);
                usleep(MODBUS_WAIT);
                int holdingRegRead3 = modbus_read_registers(ctx, GW_HREG_UTIL_END_HOUR, 1, &nEndHour);
                usleep(MODBUS_WAIT);
                
                if(-1 == inputRegRead ||
                   -1 == holdingRegRead1 ||
                   -1 == holdingRegRead2 ||
                   -1 == holdingRegRead3 )
                {
                    printft("Failed to read MODBUS registers");
                    break;
                }
                else
                {
                    if(bDumpInputRegs)
                    {
                        bDumpInputRegs = false;
                        
                        for (int j = 0; j < INPUT_REGISTER_COUNT * NUM_INVERTERS; j += 5)
                        {
                            printf("[%d]", j);
                        
                            int remaining = (INPUT_REGISTER_COUNT * NUM_INVERTERS) - j;
                            int cols = remaining < 5 ? remaining : 5;

                            for (int k = 0; k < cols; k++)
                            {
                                printf("%d\t", inputRegs[j + k]);
                            }
                            printf("\n");
                        }
                    }
                    
                    //Store relevant input register values.
                    status.nInverterState = inputRegs[STATUS];
                    status.nSolarVolts = inputRegs[PV1_VOLTS];
                    status.nSolarWatts = inputRegs[PV1_CHARGE_WATTS_L];
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
                    status.nSolarToday = inputRegs[EGY1GEN_TODAY_L];
                    status.nAcchgegyToday = inputRegs[ACCHGEGY_TODAY_L];
                    status.nBattuseToday = inputRegs[BATTUSE_TODAY_L];
                    status.nAcUseToday = inputRegs[AC_USE_TODAY_L];
                    status.nBattchgAmps = inputRegs[BATTCHG_AMPS];
                    status.nAcUseWatts = inputRegs[AC_USE_WATTS_L];
                    status.nBattUseWatts = inputRegs[BATTUSE_WATTS_L];
                    status.nBattWatts = inputRegs[BATT_WATTS_L];
                    status.nInvFanspeed = inputRegs[INV_FANSPEED];
                    
                    if(bLogging)
                    {
                        if (lMin % 5 == 0 && lMin != lLoggingLastMin)
                        {
                            //Log all values to files every five minutes.
                            printftlog("InverterState", "%d\n", status.nInverterState);
                            printftlog("nSolarVolts", "%d\n", status.nSolarVolts);
                            printftlog("nSolarWatts", "%d\n", status.nSolarWatts);
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
                            printftlog("nSolarToday", "%d\n", status.nSolarToday);
                            printftlog("AcchgegyToday", "%d\n", status.nAcchgegyToday);
                            printftlog("BattuseToday", "%d\n", status.nBattuseToday);
                            printftlog("AcUseToday", "%d\n", status.nAcUseToday);
                            printftlog("BattchgAmps", "%d\n", status.nBattchgAmps);
                            printftlog("AcUseWatts", "%d\n", status.nAcUseWatts);
                            printftlog("BattuseWatts", "%d\n", status.nBattUseWatts);
                            printftlog("BattWatts", "%d\n", status.nBattWatts);
                            printftlog("InvFanspeed", "%d\n", status.nInvFanspeed);
                        
                            lLoggingLastMin = lMin;
                        }
                    }
                }
                
                //Do general processing if reading the inverters went okay, using the values read from the master inverter.
                if(PROCESS == modbusState)
                {
                    //Find and record the off-peak charge completion time.
                    if(SYSTEM_STATE_OFF_PEAK == status.nSystemState)
                    {
                        if(status.nBattchgAmps > 0 && status.nBattchgAmps == 0)
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
                            //TO DO: Stop guessing and read the other inverters!
                            status.nOffPeakChargeKwh = status.nAcchgegyToday * INVERTER_COUNT;
                        
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
                                
                                if(status.nChargeCurrent != nChargeAmps)
                                {
                                    SetOvernightAmps();
                                    printft("Charging amps not set as expected. Rewrote holding register.\n");
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
                                
                                if(status.nChargeCurrent != nChargeAmps)
                                {
                                    SetBoostAmps();
                                    printft("Charging amps not as expected. Rewrote holding register.\n");
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
        
        usleep(MODBUS_WAIT);
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
                        printf("nSolarVolts\t%d\n", status.nSolarVolts);
                        printf("nSolarWatts\t%d\n", status.nSolarWatts);
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
                        printf("nSolarToday\t%d\n", status.nSolarToday);
                        printf("nAcchgegyToday\t%d\n", status.nAcchgegyToday);
                        printf("nBattuseToday\t%d\n", status.nBattuseToday);
                        printf("nAcUseToday\t%d\n", status.nAcUseToday);
                        printf("nBattchgAmps\t%d\n", status.nBattchgAmps);
                        printf("nAcUseWatts\t%d\n", status.nAcUseWatts);
                        printf("nBattuseWatts\t%d\n", status.nBattUseWatts);
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
                    
                    case 'f':
                    {
                        printf("Forcing boost...\n");
                        bManualSwitchToBoost = true;
                    }
                    break;
                    
                    case 'l':
                    {
                        bLogging = !bLogging;
                        printf(bLogging? "Logging\n" : "Stopped logging\n");
                    }
                    break;
                    
                    case 'm':
                    {
                        bMODBUSDebug = !bMODBUSDebug;
                        printf(bMODBUSDebug? "MODBUS debugging on next connection\n" :
                                             "MODBUS no debugging on next connection\n");
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
                    
                    case 'd':
                    {
                        printf("Dumping input regs...\n");
                        bDumpInputRegs = true;
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
                        printf("l - Toggle logging\n");
                        printf("m - Toggle MODBUS debug\n");
                        printf("d - Dump next input registers\n");
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





























