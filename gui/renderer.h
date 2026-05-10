#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "shared.h"

#define WINDOW_WIDTH     900
#define WINDOW_HEIGHT    700
#define REFRESH_MS        50

typedef struct {
    int    active;
    int    lane;
    float  x, y;
    float  tx, ty;
    float  speed;
} AnimVehicle;

#define MAX_ANIM_VEHICLES 32

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
    TTF_Font     *font_large;
    Uint32        start_ticks;
    int           emergency_flash;
    Uint32        last_flash_tick;
    AnimVehicle   anim_vehicles[MAX_ANIM_VEHICLES];
    int           prev_passed;
    int           lane_history[NUM_LANES][30];
    int           history_idx;
    Uint32        last_history_tick;
} RenderContext;

int  renderer_init(RenderContext *ctx);
void renderer_draw(RenderContext *ctx, SharedState *state);
void renderer_destroy(RenderContext *ctx);
#endif