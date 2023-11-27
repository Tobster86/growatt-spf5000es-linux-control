
#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <stdbool.h>

bool tcpclient_GetConnected();
void tcpclient_SendCommand(uint16_t nCommandID);
bool tcpclient_init();


#endif
