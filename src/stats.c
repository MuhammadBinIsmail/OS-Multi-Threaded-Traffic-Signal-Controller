#include <stdio.h>
#include "stats.h"

void print_stats(SharedState *state) {
    int i;
    double avg_wait = 0.0;

    pthread_mutex_lock(&state->stats_mutex);

    if (state->total_vehicles_passed > 0) {
        avg_wait = state->total_wait_time_sec / state->total_vehicles_passed;
    }
    printf("\n========== TRAFFIC STATISTICS ==========\n");
    printf("Total vehicles passed : %d\n", state->total_vehicles_passed);
    printf("Average wait time     : %.2f seconds\n", avg_wait);
    printf("Throughput            : %.1f vehicles/min\n", state->throughput_per_min);
    printf("-----------------------------------------\n");

    for (i = 0; i < NUM_LANES; i++) {
        printf("  %-6s lane : %d vehicles\n",
               lane_name(i), state->vehicles_per_lane[i]);
    }
    printf("-----------------------------------------\n\n");

    pthread_mutex_unlock(&state->stats_mutex);
}

void reset_stats(SharedState *state) {
    int i;
    pthread_mutex_lock(&state->stats_mutex);

    state->total_vehicles_passed = 0;
    state->total_wait_time_sec   = 0.0;
    state->throughput_per_min    = 0.0;

    for (i = 0; i < NUM_LANES; i++) {
        state->vehicles_per_lane[i] = 0;
    }
    pthread_mutex_unlock(&state->stats_mutex);
}