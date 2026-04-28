#ifndef INTERSECTION_H
#define INTERSECTION_H
 
#include "shared.h"
#include <pthread.h>
#include <semaphore.h>

int calculate_green_duration(int queue_count);
 
void process_vehicles(int lane_id, SharedState* state, int duration_ms);

int check_emergency_priority(int lane_id, SharedState* state);

void execute_normal_cycle(int lane_id, SharedState* state);

void execute_emergency_cycle(int lane_id, SharedState* state);

void run_intersection_control(int lane_id, SharedState* state);

#endif 