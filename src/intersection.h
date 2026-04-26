/***********************************************************************
 * File: intersection.h
 * Author: Adnan Osama (24K-0598)
 * Description: Intersection control logic - manages traffic light cycle,
 *              semaphore coordination, adaptive timing, and emergency
 *              vehicle priority for the multi-threaded traffic controller.
 * 
 * Responsibility: Core intersection synchronization that ensures only
 *                 one lane can cross at a time while preventing deadlock
 *                 and starvation.
 ***********************************************************************/

#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "shared.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

/***********************************************************************
 * CONSTANTS
 ***********************************************************************/

// Signal states
typedef enum {
    RED,
    YELLOW,
    GREEN
} SignalState;

// Timing constants (in milliseconds)
#define RED_DURATION       1000     // 1 second
#define YELLOW_DURATION    2000     // 2 seconds
#define BASE_GREEN_DURATION 3000    // 3 seconds base
#define GREEN_EXTENSION_PER_VEHICLE 400  // +400ms per waiting vehicle
#define MAX_GREEN_DURATION 6000     // Maximum 6 seconds

// Emergency priority
#define EMERGENCY_GREEN_DURATION 5000  // 5 seconds for emergency vehicle

/***********************************************************************
 * FUNCTION DECLARATIONS
 ***********************************************************************/

/**
 * Calculate adaptive green phase duration based on queue length
 * 
 * Formula: BASE_GREEN + (queue_length * GREEN_EXTENSION_PER_VEHICLE)
 * Capped at MAX_GREEN_DURATION
 * 
 * @param queue_length Number of vehicles waiting in the lane
 * @return Duration in milliseconds
 */
int calculate_green_duration(int queue_length);

/**
 * Process vehicles during green phase
 * 
 * Safely removes vehicles from the lane queue and updates statistics.
 * Uses mutex protection to prevent race conditions.
 * 
 * @param lane_id Lane index (0=North, 1=South, 2=East, 3=West)
 * @param state Pointer to shared state structure
 * @param duration_ms How long the green light lasts
 */
void process_vehicles(int lane_id, SharedState* state, int duration_ms);

/**
 * Check if emergency vehicle is requesting priority
 * 
 * Thread-safe check of the emergency_active flag and target lane.
 * 
 * @param lane_id Which lane to check
 * @param state Pointer to shared state structure
 * @return true if this lane has emergency priority, false otherwise
 */
bool check_emergency_priority(int lane_id, SharedState* state);

/**
 * Execute normal traffic light cycle for a lane
 * 
 * Sequence: RED → wait for semaphore → GREEN → process vehicles → 
 *           YELLOW → release semaphore → RED
 * 
 * This is the core coordination logic. Only one lane can hold
 * the intersection semaphore at a time.
 * 
 * @param lane_id Lane index (0=North, 1=South, 2=East, 3=West)
 * @param state Pointer to shared state structure
 */
void execute_normal_cycle(int lane_id, SharedState* state);

/**
 * Execute emergency override cycle for a lane
 * 
 * Bypasses normal rotation and immediately grants green light.
 * Called when emergency vehicle is detected.
 * 
 * @param lane_id Lane index (0=North, 1=South, 2=East, 3=West)
 * @param state Pointer to shared state structure
 */
void execute_emergency_cycle(int lane_id, SharedState* state);

/**
 * Main intersection control loop
 * 
 * This is the entry point called by each lane thread.
 * Continuously checks for emergency priority and executes
 * the appropriate cycle until the system shuts down.
 * 
 * @param lane_id Lane index (0=North, 1=South, 2=East, 3=West)
 * @param state Pointer to shared state structure
 */
void run_intersection_control(int lane_id, SharedState* state);

#endif // INTERSECTION_H
