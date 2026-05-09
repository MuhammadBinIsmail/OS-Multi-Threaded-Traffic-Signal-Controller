#include <stdio.h>
#include <math.h>
#include "renderer.h"

static void draw_circle(SDL_Renderer *r, int cx, int cy, int radius) {
    int x = radius;
    int y = 0;
    int err = 0;
    while (x >= y) {
        SDL_RenderDrawLine(r, cx - x, cy + y, cx + x, cy + y);
        SDL_RenderDrawLine(r, cx - x, cy - y, cx + x, cy - y);
        SDL_RenderDrawLine(r, cx - y, cy + x, cx + y, cy + x);
        SDL_RenderDrawLine(r, cx - y, cy - x, cx + y, cy - x);
        if (err <= 0) { y++; err += 2 * y + 1; }
        if (err > 0)  { x--; err -= 2 * x + 1; }
    }
}

static void draw_signal(SDL_Renderer *r, int x, int y, SignalState sig) {
    SDL_Color red    = {200,  50,  50, 255};
    SDL_Color yellow = {220, 200,  50, 255};
    SDL_Color green  = { 50, 200,  50, 255};
    SDL_Color dark   = { 50,  50,  50, 255};

    SDL_Color colors[3] = {dark, dark, dark};

    if (sig == RED)    colors[0] = red;
    if (sig == YELLOW) colors[1] = yellow;
    if (sig == GREEN)  colors[2] = green;

    int i;
    for (i = 0; i < 3; i++) {
        SDL_SetRenderDrawColor(r, colors[i].r, colors[i].g, colors[i].b, 255);
        draw_circle(r, x + 15, y + i * 40 + 15, 14);
    }
}

static void draw_text(RenderContext *ctx, const char *text, int x, int y,
                      SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Solid(ctx->font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
    if (!texture) { SDL_FreeSurface(surface); return; }
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_RenderCopy(ctx->renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

int renderer_init(RenderContext *ctx) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return -1;
    }
    if (TTF_Init() != 0) {
        printf("TTF_Init error: %s\n", TTF_GetError());
        return -1;
    }

    ctx->window = SDL_CreateWindow(
        "Traffic Signal Controller",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0
    );
    if (!ctx->window) {
        printf("Window error: %s\n", SDL_GetError());
        return -1;
    }

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ctx->renderer) {
        printf("Renderer error: %s\n", SDL_GetError());
        return -1;
    }

    ctx->font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
    if (!ctx->font) {
        ctx->font = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 14);
    }
    if (!ctx->font) {
        printf("Font error: %s\n", TTF_GetError());
        return -1;
    }

    ctx->font_large = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18);
    if (!ctx->font_large) {
        ctx->font_large = ctx->font;
    }

    return 0;
}

void renderer_draw(RenderContext *ctx, SharedState *state) {
    SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 20, 255);
    SDL_RenderClear(ctx->renderer);

    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color red    = {220,  50,  50, 255};

    int positions[NUM_LANES][2] = {
        {350, 50},    
        {350, 400},   
        {50,  250},   
        {650, 250}    
    };

    char buf[64];
    int i;
    for (i = 0; i < NUM_LANES; i++) {
        draw_signal(ctx->renderer,
                    positions[i][0],
                    positions[i][1],
                    state->signal[i]);

        snprintf(buf, sizeof(buf), "%s  Q:%d",
                 lane_name(i), state->queue_count[i]);
        draw_text(ctx, buf,
                  positions[i][0] - 10,
                  positions[i][1] + 130,
                  white);

        if (state->emergency_active &&
            state->emergency_lane == i) {
            draw_text(ctx, "!! EMERGENCY !!",
                      positions[i][0] - 30,
                      positions[i][1] - 35,
                      red);
        }
    }

    snprintf(buf, sizeof(buf), "Total passed : %d",
             state->total_vehicles_passed);
    draw_text(ctx, buf, 20, 20, white);

    snprintf(buf, sizeof(buf), "Throughput   : %.1f v/min",
             state->throughput_per_min);
    draw_text(ctx, buf, 20, 40, white);

    snprintf(buf, sizeof(buf), "Avg wait     : %.2f s",
             state->total_vehicles_passed > 0
             ? state->total_wait_time_sec / state->total_vehicles_passed
             : 0.0);
    draw_text(ctx, buf, 20, 60, white);

    SDL_RenderPresent(ctx->renderer);
}

void renderer_destroy(RenderContext *ctx) {
    if (ctx->font_large && ctx->font_large != ctx->font)
        TTF_CloseFont(ctx->font_large);
    if (ctx->font)     TTF_CloseFont(ctx->font);
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window)   SDL_DestroyWindow(ctx->window);
    TTF_Quit();
    SDL_Quit();
}