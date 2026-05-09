#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lane.h"

long compute_green_duration(int queue_count) {
    double density_factor = 1.0;
    double raw = BASE_GREEN_MS + 
                 (density_factor * queue_count * queue_count * 120.0);
    if (raw > MAX_GREEN_MS) raw = MAX_GREEN_MS;
    if (raw < BASE_GREEN_MS) raw = BASE_GREEN_MS;
    return (long)raw;
}
static void ms_sleep(long ms) {
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}
static double timespec_diff_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec  - start.tv_sec) +
           (end.tv_nsec - start.tv_nsec) / 1e9;
}
void *lane_thread(void *arg) {
    LaneArgs    *args  = (LaneArgs *)arg;
    SharedState *state = args->state;
    int          id    = args->lane_id;

    printf("%s lane thread started.\n", lane_name(id));

    while (state->running) {
        pthread_mutex_lock(&state->emergency_mutex);
        int emg = state->emergency_active;
        int emg_lane = state->emergency_lane;
        pthread_mutex_unlock(&state->emergency_mutex);

        if (emg && emg_lane != id) {
            state->signal[id] = RED;
            ms_sleep(RED_SLEEP_MS);
            continue;
        }
        pthread_mutex_lock(&state->queue_mutex[id]);
        int waiting = state->queue_count[id];
        pthread_mutex_unlock(&state->queue_mutex[id]);

        if (waiting == 0) {
            state->signal[id] = RED;
            ms_sleep(RED_SLEEP_MS);
            continue;
        }
        sem_wait(&state->intersection_sem);

        if (!state->running) {
            sem_post(&state->intersection_sem);
            break;
        }
        pthread_mutex_lock(&state->emergency_mutex);
        emg = state->emergency_active;
        pthread_mutex_unlock(&state->emergency_mutex);

        if (emg) {
            sem_post(&state->intersection_sem);
            state->signal[id] = RED;
            ms_sleep(RED_SLEEP_MS);
            continue;
        }
        long green_ms = compute_green_duration(waiting);
        state->signal[id] = GREEN;
        printf("%s lane GREEN | Queue: %d | Green time: %ld ms\n",
               lane_name(id), waiting, green_ms);

        long elapsed = 0;
        while (elapsed < green_ms && state->running) {

            pthread_mutex_lock(&state->emergency_mutex);
            emg = state->emergency_active;
            pthread_mutex_unlock(&state->emergency_mutex);
            if (emg) break;

            pthread_mutex_lock(&state->queue_mutex[id]);
            if (state->queue_count[id] > 0) {

                Vehicle v = state->lane_queue[id][0];
                int i;
                for (i = 0; i < state->queue_count[id] - 1; i++) {
                    state->lane_queue[id][i] = state->lane_queue[id][i + 1];
                }
                state->queue_count[id]--;

                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                double wait = timespec_diff_sec(v.arrival_time, now);

                pthread_mutex_unlock(&state->queue_mutex[id]);
                pthread_mutex_lock(&state->stats_mutex);
                state->total_vehicles_passed++;
                state->vehicles_per_lane[id]++;
                state->total_wait_time_sec += wait;
                double secs = elapsed_seconds(state);
                if (secs > 0)
                    state->throughput_per_min =
                        (state->total_vehicles_passed / secs) * 60.0;
                pthread_mutex_unlock(&state->stats_mutex);

                printf("%s lane: Vehicle %d crossed | Wait: %.2f s | "
                       "Throughput: %.1f/min\n",
                       lane_name(id), v.vehicle_id, wait,
                       state->throughput_per_min);
            } else {
                pthread_mutex_unlock(&state->queue_mutex[id]);
            }

            ms_sleep(500);
            elapsed += 500;
        }
        state->signal[id] = YELLOW;
        printf("%s lane YELLOW\n", lane_name(id));
        ms_sleep(YELLOW_MS);

        state->signal[id] = RED;
        sem_post(&state->intersection_sem);
        printf("%s lane RED — semaphore released.\n", lane_name(id));
    }
    printf("%s lane thread stopped.\n", lane_name(id));
    return NULL;
}