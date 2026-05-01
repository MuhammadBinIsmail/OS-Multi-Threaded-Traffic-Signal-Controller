#ifndef LANE_H
#define LANE_H

#include "shared.h"

typedef struct {
    SharedState *state;
    int lane_id;
} LaneArgs;

long compute_green_duration(int queue_count);
void *lane_thread(void *arg);

#endif