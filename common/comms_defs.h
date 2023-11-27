
#ifndef COMMS_DEFS_H
#define COMMS_DEFS_H

#include <stdint.h>

/* Commands */
#define COMMAND_REQUEST_STATUS  0x0001

/* Objects */
#define OBJECT_STATUS           0x0001

extern void GetStatus(uint8_t** ppStatus, uint32_t* pLength);
extern void ReceiveStatus(uint8_t* pStatus, uint32_t lLength);

#endif

