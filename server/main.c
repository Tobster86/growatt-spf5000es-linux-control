
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

#define INPUT_REG_COUNT 83

long long current_time_millis()
{
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}

long long oTimer;
long long oTimeEnd;
uint32_t lCounts = 0;

int main()
{  
    modbus_t *ctx;
    int rc;
    uint16_t tab_reg[INPUT_REG_COUNT]; // To store the read data

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
        // Read 100 lines of input registers starting from address 0
        rc = modbus_read_input_registers(ctx, 0, INPUT_REG_COUNT, tab_reg);
        if (rc == -1) {
            fprintf(stderr, "Read registers failed: %s\n", modbus_strerror(errno));
            modbus_free(ctx);
            return -1;
        }

        // Print the response
        for (int i = 0; i < INPUT_REG_COUNT; i++) {
            printf("Register %d: %d\n", i, tab_reg[i]);
        }
        
        lCounts++;
        oTimer = current_time_millis();
    }
    
    printf("%d hits in %lldms.\n", lCounts, oTimer - oTimeEnd + 1000);

    // Disconnect and cleanup
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}





























