#include "intersection.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define msleep(ms) usleep((ms) * 1000)

int calculate_green_duration(int queue_count) {

    int duration = BASE_GREEN_MS + (queue_count * DENSITY_SCALE);
    
    if (duration > MAX_GREEN_MS) {
        duration = MAX_GREEN_MS;
    }
    
    return duration;
}

void process_vehicles(int lane_id, SharedState* state, int duration_ms) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    
   
    pthread_mutex_lock(&state->queue_mutex[lane_id]);
    
    int queue_count = state->queue_count[lane_id];
    int vehicles_processed = 0;
    
  
    int max_vehicles = duration_ms / 1000;
    if (max_vehicles < 1) max_vehicles = 1;
    

    if (queue_count > 0) {
        vehicles_processed = (queue_count < max_vehicles) ? queue_count : max_vehicles;
        
        printf("[%s Lane] GREEN - Processing %d vehicles (queue: %d → %d)\n", 
               lane_name(lane_id), vehicles_processed, queue_count, 
               queue_count - vehicles_processed);
        
      
        double total_wait = 0.0;
        for (int i = 0; i < vehicles_processed; i++) {
            Vehicle v = state->lane_queue[lane_id][i];
            
      
            double wait_sec = (now.tv_sec - v.arrival_time.tv_sec) +
                             (now.tv_nsec - v.arrival_time.tv_nsec) / 1e9;
            total_wait += wait_sec;
            
           
            printf("  → Vehicle %d passed (waited %.2f seconds)\n", 
                   v.vehicle_id, wait_sec);
        }
        

        int remaining = queue_count - vehicles_processed;
        if (remaining > 0) {
            memmove(&state->lane_queue[lane_id][0],
                   &state->lane_queue[lane_id][vehicles_processed],
                   remaining * sizeof(Vehicle));
        }
        

        state->queue_count[lane_id] = remaining;
        
        pthread_mutex_unlock(&state->queue_mutex[lane_id]);
        

        pthread_mutex_lock(&state->stats_mutex);
        state->total_vehicles_passed += vehicles_processed;
        state->vehicles_per_lane[lane_id] += vehicles_processed;
        state->total_wait_time_sec += total_wait;
        

        double elapsed = elapsed_seconds(state);
        if (elapsed > 0) {
            state->throughput_per_min = (state->total_vehicles_passed / elapsed) * 60.0;
        }
        pthread_mutex_unlock(&state->stats_mutex);
        
    } else {
        pthread_mutex_unlock(&state->queue_mutex[lane_id]);
        printf("[%s Lane] GREEN - No vehicles waiting\n", lane_name(lane_id));
    }
    

    msleep(duration_ms);
}

int check_emergency_priority(int lane_id, SharedState* state) {
    pthread_mutex_lock(&state->emergency_mutex);
    int is_emergency = (state->emergency_active && 
                       state->emergency_lane == lane_id);
    pthread_mutex_unlock(&state->emergency_mutex);
    
    return is_emergency;
}

void execute_normal_cycle(int lane_id, SharedState* state) {

    state->signal[lane_id] = RED;
    printf("[%s Lane] RED - Waiting for intersection access\n", lane_name(lane_id));
    
    sem_wait(&state->intersection_sem);
    
    state->signal[lane_id] = GREEN;
    
    pthread_mutex_lock(&state->queue_mutex[lane_id]);
    int queue_count = state->queue_count[lane_id];
    pthread_mutex_unlock(&state->queue_mutex[lane_id]);
    
    int green_duration = calculate_green_duration(queue_count);
    printf("[%s Lane] GREEN phase starting (duration: %d ms, queue: %d vehicles)\n",
           lane_name(lane_id), green_duration, queue_count);
    
    process_vehicles(lane_id, state, green_duration);
    
    state->signal[lane_id] = YELLOW;
    printf("[%s Lane] YELLOW - Transition phase\n", lane_name(lane_id));
    msleep(YELLOW_MS);
    
    state->signal[lane_id] = RED;
    sem_post(&state->intersection_sem);
    printf("[%s Lane] RED - Released intersection\n", lane_name(lane_id));
    
    msleep(RED_SLEEP_MS);
}

void execute_emergency_cycle(int lane_id, SharedState* state) {
    printf("\n*** [%s Lane] EMERGENCY VEHICLE DETECTED! ***\n", lane_name(lane_id));
  
    sem_wait(&state->intersection_sem);
   
    state->signal[lane_id] = GREEN;
    printf("[%s Lane] EMERGENCY GREEN - Priority access granted\n", lane_name(lane_id));
   
    int emergency_duration = BASE_GREEN_MS + EMERGENCY_WAIT_MS;
    process_vehicles(lane_id, state, emergency_duration);

    state->signal[lane_id] = YELLOW;
    msleep(YELLOW_MS);

    state->signal[lane_id] = RED;
    sem_post(&state->intersection_sem);
   
    pthread_mutex_lock(&state->emergency_mutex);
    state->emergency_active = 0;
    state->emergency_lane = -1;
    pthread_mutex_unlock(&state->emergency_mutex);
    
    printf("[%s Lane] EMERGENCY complete - Normal operation resumed\n\n", lane_name(lane_id));
}

void run_intersection_control(int lane_id, SharedState* state) {
    printf("[%s Lane] Intersection control started\n", lane_name(lane_id));
    
    while (state->running) {

        if (check_emergency_priority(lane_id, state)) {
            execute_emergency_cycle(lane_id, state);
        } 

        else if (state->emergency_active) {

            printf("[%s Lane] Yielding to emergency vehicle\n", lane_name(lane_id));
            msleep(500); 
        }

        else {
            execute_normal_cycle(lane_id, state);
        }
    }
    
    printf("[%s Lane] Intersection control stopped\n", lane_name(lane_id));
}