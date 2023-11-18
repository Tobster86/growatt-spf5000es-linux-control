
#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <stdbool.h>

/**
 * Call to initialise and run the process thread.
 */
bool tcpserver_init();

/**
 * Call to instruct the process thread to deinitialise and terminate.
 */
void tcpserver_deinit();

#endif
