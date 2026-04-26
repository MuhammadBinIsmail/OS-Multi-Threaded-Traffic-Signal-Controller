#ifndef VEHICLE_GEN_H
#define VEHICLE_GEN_H

#include "shared.h"

#define MIN_ARRIVAL_INTERVAL_MS 500
#define MAX_ARRIVAL_INTERVAL_MS 2000

void *vehicle_generator_thread(void *arg);

#endif
