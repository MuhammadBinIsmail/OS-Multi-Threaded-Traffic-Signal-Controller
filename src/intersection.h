#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "shared.h"

long calculate_green_duration(int queue_count);
void process_vehicles(int lane_id, SharedState *state, int duration_ms);
int  check_emergency_priority(int lane_id, SharedState *state);

#endif