#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "shared.h"
#include "lane.h"
#include "stats.h"
#include "intersection.h"
#include "vehicle_gen.h"
#include "emergency.h"
#include "../gui/renderer.h"

static SharedState state;

void handle_shutdown(int sig) {
    (void)sig;
    printf("\nShutting down...\n");
    state.running = 0;
    sem_post(&state.intersection_sem);
}

int main(void) {
    pthread_t lane_threads[NUM_LANES];
    pthread_t gen_thread;
    pthread_t emg_thread;
    LaneArgs  lane_args[NUM_LANES];
    RenderContext ctx;
    int i;

    signal(SIGINT, handle_shutdown);

    shared_state_init(&state);

    if (renderer_init(&ctx) != 0) {
        printf("Failed to init renderer. Exiting.\n");
        shared_state_destroy(&state);
        return 1;
    }

    for (i = 0; i < NUM_LANES; i++) {
        lane_args[i].state   = &state;
        lane_args[i].lane_id = i;
        pthread_create(&lane_threads[i], NULL, lane_thread, &lane_args[i]);
    }

    pthread_create(&gen_thread, NULL, vehicle_generator_thread, &state);
    pthread_create(&emg_thread, NULL, emergency_thread,         &state);

    printf("All threads started. Running simulation...\n\n");

    SDL_Event event;
    while (state.running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                state.running = 0;
            }
        }

        renderer_draw(&ctx, &state);

        struct timespec ts;
        ts.tv_sec  = REFRESH_MS / 1000;
        ts.tv_nsec = (REFRESH_MS % 1000) * 1000000L;
        nanosleep(&ts, NULL);

        print_stats(&state);
    }

    for (i = 0; i < NUM_LANES; i++) {
        pthread_join(lane_threads[i], NULL);
    }
    pthread_join(gen_thread, NULL);
    pthread_join(emg_thread, NULL);

    renderer_destroy(&ctx);
    shared_state_destroy(&state);

    return 0;
}