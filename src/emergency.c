#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "emergency.h"

void *emergency_thread(void *arg) {
    SharedState *state = (SharedState *)arg;
    srand(time(NULL) + 1);

    int next_ev_id = 1000;

    while (state->running) {

        int interval_ms = MIN_EMERGENCY_INTERVAL_MS +
                          rand() % (MAX_EMERGENCY_INTERVAL_MS - MIN_EMERGENCY_INTERVAL_MS);

        struct timespec sleep_time;
        sleep_time.tv_sec  = interval_ms / 1000;
        sleep_time.tv_nsec = (interval_ms % 1000) * 1000000L;
        nanosleep(&sleep_time, NULL);

        if (!state->running) break;

        int target_lane = rand() % NUM_LANES;
        int priority    = 1 + rand() % 3;

        EmergencyVehicle ev;
        ev.vehicle_id = next_ev_id++;
        ev.lane       = target_lane;
        ev.priority   = priority;

        pthread_mutex_lock(&state->emergency_mutex);
        pq_push(&state->emergency_pq, ev);
        pthread_mutex_unlock(&state->emergency_mutex);

        printf("EMERGENCY: Vehicle %d queued for %s lane (priority %d)\n",
               ev.vehicle_id, lane_name(target_lane), priority);

        pthread_mutex_lock(&state->emergency_mutex);
        if (!pq_is_empty(&state->emergency_pq)) {

            EmergencyVehicle top = pq_pop(&state->emergency_pq);
            state->emergency_active = 1;
            state->emergency_lane   = top.lane;
            pthread_mutex_unlock(&state->emergency_mutex);

            printf("EMERGENCY: Forcing %s lane GREEN for vehicle %d (priority %d)\n",
                   lane_name(top.lane), top.vehicle_id, top.priority);

            sem_wait(&state->intersection_sem);
            state->signal[top.lane] = GREEN;

            sleep_time.tv_sec  = EMERGENCY_WAIT_MS / 1000;
            sleep_time.tv_nsec = (EMERGENCY_WAIT_MS % 1000) * 1000000L;
            nanosleep(&sleep_time, NULL);

            state->signal[top.lane] = RED;
            sem_post(&state->intersection_sem);

            pthread_mutex_lock(&state->emergency_mutex);
            state->emergency_active = 0;
            state->emergency_lane   = -1;
            pthread_mutex_unlock(&state->emergency_mutex);

            printf("EMERGENCY: %s lane cleared. Resuming normal cycle.\n",
                   lane_name(top.lane));

        } else {
            pthread_mutex_unlock(&state->emergency_mutex);
        }
    }

    printf("Emergency thread stopped.\n");
    return NULL;
}
