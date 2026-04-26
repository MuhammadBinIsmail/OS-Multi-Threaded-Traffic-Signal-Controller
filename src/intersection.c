/***********************************************************************
 * File: intersection.c
 * Author: Adnan Osama (24K-0598)
 * Description: Implementation of intersection control logic.
 * 
 * This file contains the core synchronization code that manages
 * the traffic intersection using POSIX semaphores and mutexes.
 ***********************************************************************/

#include "intersection.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Helper macro for sleeping in milliseconds
#define msleep(ms) usleep((ms) * 1000)

// Lane names for logging
static const char* LANE_NAMES[] = {"North", "South", "East", "West"};

/***********************************************************************
 * ADAPTIVE TIMING CALCULATION
 ***********************************************************************/

int calculate_green_duration(int queue_length) {
    // Formula: BASE + (queue * EXTENSION), capped at MAX
    int duration = BASE_GREEN_DURATION + (queue_length * GREEN_EXTENSION_PER_VEHICLE);
    
    // Cap at maximum duration
    if (duration > MAX_GREEN_DURATION) {
        duration = MAX_GREEN_DURATION;
    }
    
    return duration;
}

/***********************************************************************
 * VEHICLE PROCESSING
 ***********************************************************************/

void process_vehicles(int lane_id, SharedState* state, int duration_ms) {
    // Lock the lane's queue to safely read/modify it
    pthread_mutex_lock(&state->queue_mutex[lane_id]);
    
    int queue_length = state->lane_queue[lane_id];
    int vehicles_processed = 0;
    
    // Calculate how many vehicles can pass during this green phase
    // Assume 1 vehicle per second throughput
    int max_vehicles = duration_ms / 1000;
    if (max_vehicles < 1) max_vehicles = 1; // At least 1 vehicle if queue exists
    
    // Process vehicles (remove from queue)
    if (queue_length > 0) {
        vehicles_processed = (queue_length < max_vehicles) ? queue_length : max_vehicles;
        state->lane_queue[lane_id] -= vehicles_processed;
        
        printf("[%s Lane] GREEN - Processing %d vehicles (%d remaining)\n", 
               LANE_NAMES[lane_id], vehicles_processed, state->lane_queue[lane_id]);
    } else {
        printf("[%s Lane] GREEN - No vehicles waiting\n", LANE_NAMES[lane_id]);
    }
    
    pthread_mutex_unlock(&state->queue_mutex[lane_id]);
    
    // Update global statistics with mutex protection
    if (vehicles_processed > 0) {
        pthread_mutex_lock(&state->stats_mutex);
        state->total_vehicles_passed += vehicles_processed;
        state->throughput_per_min += vehicles_processed;
        pthread_mutex_unlock(&state->stats_mutex);
    }
    
    // Simulate the green phase duration
    msleep(duration_ms);
}

/***********************************************************************
 * EMERGENCY PRIORITY CHECK
 ***********************************************************************/

bool check_emergency_priority(int lane_id, SharedState* state) {
    pthread_mutex_lock(&state->emergency_mutex);
    bool is_emergency = (state->emergency_active && 
                        state->emergency_target_lane == lane_id);
    pthread_mutex_unlock(&state->emergency_mutex);
    
    return is_emergency;
}

/***********************************************************************
 * NORMAL TRAFFIC CYCLE
 ***********************************************************************/

void execute_normal_cycle(int lane_id, SharedState* state) {
    // PHASE 1: RED - Wait for our turn
    state->signal[lane_id] = RED;
    printf("[%s Lane] RED - Waiting for intersection access\n", LANE_NAMES[lane_id]);
    
    // Wait for the intersection semaphore (blocking)
    // This ensures only ONE lane can cross at a time
    sem_wait(&state->intersection_sem);
    
    // We now have exclusive access to the intersection
    
    // PHASE 2: GREEN - Process vehicles with adaptive timing
    state->signal[lane_id] = GREEN;
    
    // Check queue length for adaptive timing
    pthread_mutex_lock(&state->queue_mutex[lane_id]);
    int queue_length = state->lane_queue[lane_id];
    pthread_mutex_unlock(&state->queue_mutex[lane_id]);
    
    int green_duration = calculate_green_duration(queue_length);
    printf("[%s Lane] GREEN phase starting (duration: %d ms, queue: %d vehicles)\n",
           LANE_NAMES[lane_id], green_duration, queue_length);
    
    // Process vehicles during green phase
    process_vehicles(lane_id, state, green_duration);
    
    // PHASE 3: YELLOW - Transition warning
    state->signal[lane_id] = YELLOW;
    printf("[%s Lane] YELLOW - Transition phase\n", LANE_NAMES[lane_id]);
    msleep(YELLOW_DURATION);
    
    // PHASE 4: Release the intersection - allow next lane
    state->signal[lane_id] = RED;
    sem_post(&state->intersection_sem);
    printf("[%s Lane] RED - Released intersection\n", LANE_NAMES[lane_id]);
}

/***********************************************************************
 * EMERGENCY OVERRIDE CYCLE
 ***********************************************************************/

void execute_emergency_cycle(int lane_id, SharedState* state) {
    printf("\n*** [%s Lane] EMERGENCY VEHICLE DETECTED! ***\n", LANE_NAMES[lane_id]);
    
    // Immediately acquire intersection access
    // Other lanes check emergency_active flag and skip their turn
    sem_wait(&state->intersection_sem);
    
    // Force green immediately
    state->signal[lane_id] = GREEN;
    printf("[%s Lane] EMERGENCY GREEN - Priority access granted\n", LANE_NAMES[lane_id]);
    
    // Process emergency vehicle with fixed duration
    process_vehicles(lane_id, state, EMERGENCY_GREEN_DURATION);
    
    // Short yellow transition
    state->signal[lane_id] = YELLOW;
    msleep(YELLOW_DURATION);
    
    // Release intersection
    state->signal[lane_id] = RED;
    sem_post(&state->intersection_sem);
    
    // Clear emergency flag
    pthread_mutex_lock(&state->emergency_mutex);
    state->emergency_active = false;
    state->emergency_target_lane = -1;
    pthread_mutex_unlock(&state->emergency_mutex);
    
    printf("[%s Lane] EMERGENCY complete - Normal operation resumed\n\n", 
           LANE_NAMES[lane_id]);
}

/***********************************************************************
 * MAIN INTERSECTION CONTROL LOOP
 ***********************************************************************/

void run_intersection_control(int lane_id, SharedState* state) {
    printf("[%s Lane] Intersection control started\n", LANE_NAMES[lane_id]);
    
    while (state->running) {
        // Check if this lane has emergency priority
        if (check_emergency_priority(lane_id, state)) {
            execute_emergency_cycle(lane_id, state);
        } 
        // Check if another lane has emergency (skip our turn if so)
        else if (state->emergency_active) {
            // Another lane has emergency priority - wait
            printf("[%s Lane] Yielding to emergency vehicle on another lane\n", 
                   LANE_NAMES[lane_id]);
            msleep(500); // Brief pause before rechecking
        }
        // Normal operation
        else {
            execute_normal_cycle(lane_id, state);
        }
        
        // Small delay before next cycle to prevent CPU spinning
        msleep(100);
    }
    
    printf("[%s Lane] Intersection control stopped\n", LANE_NAMES[lane_id]);
}
