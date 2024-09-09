
#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <stdbool.h>
#include <stdint.h>

bool tcpclient_GetConnected();
void tcpclient_SendCommand(uint16_t nCommandID);
bool tcpclient_init();

extern void _tcpclient_ReceiveStatus(uint8_t* pStatus, uint32_t lLength);

#endif
