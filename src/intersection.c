#include <stdio.h>
#include <time.h>
#include <string.h>
#include "intersection.h"

static void ms_sleep(long ms) {
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

long calculate_green_duration(int queue_count) {
    double raw = BASE_GREEN_MS +
                 (1.0 * queue_count * queue_count * 120.0);
    if (raw > MAX_GREEN_MS) raw = MAX_GREEN_MS;
    if (raw < BASE_GREEN_MS) raw = BASE_GREEN_MS;
    return (long)raw;
}

void process_vehicles(int lane_id, SharedState *state, int duration_ms) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    pthread_mutex_lock(&state->queue_mutex[lane_id]);

    int queue_count = state->queue_count[lane_id];
    int max_vehicles = duration_ms / 1000;
    if (max_vehicles < 1) max_vehicles = 1;

    int vehicles_processed = 0;

    if (queue_count > 0) {
        vehicles_processed = (queue_count < max_vehicles)
                             ? queue_count : max_vehicles;

        double total_wait = 0.0;
        int i;
        for (i = 0; i < vehicles_processed; i++) {
            Vehicle v = state->lane_queue[lane_id][i];
            double wait_sec = (now.tv_sec  - v.arrival_time.tv_sec) +
                              (now.tv_nsec - v.arrival_time.tv_nsec) / 1e9;
            total_wait += wait_sec;
            printf("  Vehicle %d passed %s lane | waited %.2f sec\n",
                   v.vehicle_id, lane_name(lane_id), wait_sec);
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
        state->total_vehicles_passed     += vehicles_processed;
        state->vehicles_per_lane[lane_id]+= vehicles_processed;
        state->total_wait_time_sec       += total_wait;
        double elapsed = elapsed_seconds(state);
        if (elapsed > 0)
            state->throughput_per_min =
                (state->total_vehicles_passed / elapsed) * 60.0;
        pthread_mutex_unlock(&state->stats_mutex);

        printf("[%s] %d vehicles processed | Queue remaining: %d\n",
               lane_name(lane_id), vehicles_processed, remaining);

    } else {
        pthread_mutex_unlock(&state->queue_mutex[lane_id]);
        printf("[%s] GREEN — no vehicles waiting\n", lane_name(lane_id));
    }

    ms_sleep(duration_ms);
}

int check_emergency_priority(int lane_id, SharedState *state) {
    pthread_mutex_lock(&state->emergency_mutex);
    int is_emergency = (state->emergency_active &&
                        state->emergency_lane == lane_id);
    pthread_mutex_unlock(&state->emergency_mutex);
    return is_emergency;
}