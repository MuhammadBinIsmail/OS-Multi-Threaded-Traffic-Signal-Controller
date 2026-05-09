#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "vehicle_gen.h"

void *vehicle_generator_thread(void *arg) {
    SharedState *state = (SharedState *)arg;
    srand(time(NULL));

    while (state->running) {

        int lane = rand() % NUM_LANES;
        int interval_ms = MIN_ARRIVAL_INTERVAL_MS +
                          rand() % (MAX_ARRIVAL_INTERVAL_MS - MIN_ARRIVAL_INTERVAL_MS);

        pthread_mutex_lock(&state->queue_mutex[lane]);

        if (state->queue_count[lane] < MAX_QUEUE) {

            Vehicle v;

            pthread_mutex_lock(&state->stats_mutex);
            v.vehicle_id = state->next_vehicle_id++;
            pthread_mutex_unlock(&state->stats_mutex);

            v.lane = lane;
            clock_gettime(CLOCK_MONOTONIC, &v.arrival_time);

            state->lane_queue[lane][state->queue_count[lane]] = v;
            state->queue_count[lane]++;

            printf("Vehicle %d added to %s lane | Queue size: %d\n",
                   v.vehicle_id,
                   lane_name(lane),
                   state->queue_count[lane]);

        } else {
            printf("%s lane full — vehicle skipped.\n", lane_name(lane));
        }

        pthread_mutex_unlock(&state->queue_mutex[lane]);

        struct timespec sleep_time;
        sleep_time.tv_sec  = interval_ms / 1000;
        sleep_time.tv_nsec = (interval_ms % 1000) * 1000000L;
        nanosleep(&sleep_time, NULL);
    }

    printf("Vehicle generator stopped.\n");
    return NULL;
}
