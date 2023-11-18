
#ifndef COMMS_DEFS_H
#define COMMS_DEFS_H

#include <stdint.h>

/* Commands */
#define COMMAND_REQUEST_STATUS  0x01

/* Objects */
#define OBJECT_STATUS           0x01

extern void GetStatus(uint8_t** ppStatus, uint32_t* pLength);

#endif

