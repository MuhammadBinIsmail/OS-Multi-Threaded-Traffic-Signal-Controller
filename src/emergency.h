#ifndef EMERGENCY_H
#define EMERGENCY_H

#include "shared.h"

#define MIN_EMERGENCY_INTERVAL_MS 15000
#define MAX_EMERGENCY_INTERVAL_MS 25000

void *emergency_thread(void *arg);

#endif
