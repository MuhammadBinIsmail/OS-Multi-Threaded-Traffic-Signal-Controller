#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared.h"

void shared_state_init(SharedState *state) {
    int i;
    memset(state, 0, sizeof(SharedState));

    state->running = 1;
    state->next_vehicle_id = 1;
    state->emergency_active = 0;
    state->emergency_lane = -1;
    state->total_vehicles_passed = 0;
    state->total_wait_time_sec = 0.0;
    state->throughput_per_min = 0.0;

    for (i = 0; i < NUM_LANES; i++) {
        state->signal[i] = RED;
        state->queue_count[i] = 0;
        state->vehicles_per_lane[i] = 0;
    }
    state->emergency_pq.size = 0;
    sem_init(&state->intersection_sem, 0, 1);

    for (i = 0; i < NUM_LANES; i++) {
        pthread_mutex_init(&state->queue_mutex[i], NULL);
    }
    pthread_mutex_init(&state->stats_mutex, NULL);
    pthread_mutex_init(&state->emergency_mutex, NULL);

    clock_gettime(CLOCK_MONOTONIC, &state->sim_start_time);

    printf("Traffic signal system initialised.\n");
    printf("Lanes: North | South | East | West\n");
    printf("Semaphore and mutexes ready.\n\n");
}
void shared_state_destroy(SharedState *state) {
    int i;
    sem_destroy(&state->intersection_sem);

    for (i = 0; i < NUM_LANES; i++) {
        pthread_mutex_destroy(&state->queue_mutex[i]);
    }
    pthread_mutex_destroy(&state->stats_mutex);
    pthread_mutex_destroy(&state->emergency_mutex);

    printf("\nSystem shutdown complete.\n");
    printf("Total vehicles passed : %d\n", state->total_vehicles_passed);
    printf("Average wait time     : %.2f seconds\n",
           state->total_vehicles_passed > 0
           ? state->total_wait_time_sec / state->total_vehicles_passed
           : 0.0);
    printf("Final throughput      : %.2f vehicles/min\n", state->throughput_per_min);
}
void pq_push(EmergencyPQ *pq, EmergencyVehicle ev) {
    int i;

    if (pq->size >= MAX_EMERGENCY_Q) {
        printf("Emergency queue full. Vehicle %d dropped.\n", ev.vehicle_id);
        return;
    }
    pq->heap[pq->size] = ev;
    i = pq->size;
    pq->size++;

    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pq->heap[parent].priority > pq->heap[i].priority) {
            EmergencyVehicle temp = pq->heap[parent];
            pq->heap[parent] = pq->heap[i];
            pq->heap[i] = temp;
            i = parent;
        } else {
            break;
        }
    }
}
EmergencyVehicle pq_pop(EmergencyPQ *pq) {
    EmergencyVehicle top = pq->heap[0];
    int i = 0;
    pq->size--;
    pq->heap[0] = pq->heap[pq->size];

    while (1) {
        int left  = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < pq->size &&
            pq->heap[left].priority < pq->heap[smallest].priority) {
            smallest = left;
        }
        if (right < pq->size &&
            pq->heap[right].priority < pq->heap[smallest].priority) {
            smallest = right;
        }
        if (smallest == i) break;

        EmergencyVehicle temp = pq->heap[smallest];
        pq->heap[smallest] = pq->heap[i];
        pq->heap[i] = temp;
        i = smallest;
    }
    return top;
}
int pq_is_empty(EmergencyPQ *pq) {
    return pq->size == 0;
}
double elapsed_seconds(SharedState *state) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - state->sim_start_time.tv_sec) +
           (now.tv_nsec - state->sim_start_time.tv_nsec) / 1e9;
}
const char *lane_name(int lane) {
    switch (lane) {
        case NORTH: return "North";
        case SOUTH: return "South";
        case EAST:  return "East";
        case WEST:  return "West";
        default:    return "Unknown";
    }
}