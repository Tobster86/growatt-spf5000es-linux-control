
#ifndef GROWATT_CONTROL_UTILS_H
#define GROWATT_CONTROL_UTILS_H

#include <stdint.h>
#include "spf5000es_defs.h"
#include "system_defs.h"

uint16_t utils_GetOffpeakChargingAmps(struct SystemStatus* pSystemStatus);

#endif

