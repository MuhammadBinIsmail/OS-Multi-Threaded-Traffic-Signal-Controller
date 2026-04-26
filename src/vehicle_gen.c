#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "vehicle_gen.h"

void *vehicle_generator_thread(void *arg) {
    SharedState *state = (SharedState *)arg;
    srand(time(NULL));

    while (state->running) {
        int lane = rand() % 4;
        int interval_ms = MIN_ARRIVAL_INTERVAL_MS +
                          rand() % (MAX_ARRIVAL_INTERVAL_MS - MIN_ARRIVAL_INTERVAL_MS);

        pthread_mutex_lock(&state->queue_mutex[lane]);
        state->lane_queue[lane]++;
        printf("Vehicle added to lane %d | Queue: %d\n", lane, state->lane_queue[lane]);
        pthread_mutex_unlock(&state->queue_mutex[lane]);

        usleep(interval_ms * 1000);
    }

    return NULL;
}
