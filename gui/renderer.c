#include <stdio.h>
#include <math.h>
#include <string.h>
#include "renderer.h"

static SDL_Color WHITE    = {255,255,255,255};
static SDL_Color RED_C    = {220, 50, 50,255};
static SDL_Color YELLOW_C = {220,200, 50,255};
static SDL_Color GREEN_C  = { 50,200, 50,255};
static SDL_Color DARK_C   = { 40, 40, 40,255};
static SDL_Color ROAD_C   = { 60, 60, 60,255};

static void fill_circle(SDL_Renderer *r, int cx, int cy, int radius) {
    int x, y;
    for (y = -radius; y <= radius; y++) {
        int dx = (int)sqrt((double)(radius*radius - y*y));
        SDL_RenderDrawLine(r, cx - dx, cy + y, cx + dx, cy + y);
    }
}

static void draw_text_col(RenderContext *ctx, TTF_Font *font,
                          const char *text, int x, int y, SDL_Color col) {
    SDL_Surface *surf = TTF_RenderText_Solid(font, text, col);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ctx->renderer, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(ctx->renderer, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static void draw_text(RenderContext *ctx, const char *text,
                      int x, int y, SDL_Color col) {
    draw_text_col(ctx, ctx->font, text, x, y, col);
}

static void draw_signal(SDL_Renderer *r, int x, int y, SignalState sig) {
    SDL_SetRenderDrawColor(r, 30, 30, 30, 255);
    SDL_Rect box = {x - 8, y - 8, 46, 116};
    SDL_RenderFillRect(r, &box);
    SDL_SetRenderDrawColor(r, 80, 80, 80, 255);
    SDL_RenderDrawRect(r, &box);

    SDL_Color colors[3] = {DARK_C, DARK_C, DARK_C};
    if (sig == RED)    colors[0] = RED_C;
    if (sig == YELLOW) colors[1] = YELLOW_C;
    if (sig == GREEN)  colors[2] = GREEN_C;

    int i;
    for (i = 0; i < 3; i++) {
        SDL_SetRenderDrawColor(r, colors[i].r, colors[i].g, colors[i].b, 255);
        fill_circle(r, x + 15, y + i * 38 + 15, 13);
    }
}

static void draw_roads(SDL_Renderer *r) {
    SDL_SetRenderDrawColor(r, ROAD_C.r, ROAD_C.g, ROAD_C.b, 255);
    SDL_Rect h = {0, 290, 900, 80};
    SDL_RenderFillRect(r, &h);
    SDL_Rect v = {390, 0, 80, 600};
    SDL_RenderFillRect(r, &v);
    SDL_SetRenderDrawColor(r, 70, 70, 70, 255);
    SDL_Rect center = {390, 290, 80, 80};
    SDL_RenderFillRect(r, &center);
    SDL_SetRenderDrawColor(r, 200, 200, 50, 100);
    SDL_Rect hm = {0, 328, 900, 4};
    SDL_RenderFillRect(r, &hm);
    SDL_Rect vm = {428, 0, 4, 600};
    SDL_RenderFillRect(r, &vm);
}

static void spawn_anim(RenderContext *ctx, int lane,
                       int positions[NUM_LANES][2]) {
    int i;
    for (i = 0; i < MAX_ANIM_VEHICLES; i++) {
        if (!ctx->anim_vehicles[i].active) {
            AnimVehicle *v = &ctx->anim_vehicles[i];
            v->active = 1;
            v->lane   = lane;
            v->x = (float)positions[lane][0];
            v->y = (float)positions[lane][1] + 60;
            v->tx = 415;
            v->ty = 315;
            v->speed = 3.0f;
            break;
        }
    }
}

static void update_anim_vehicles(RenderContext *ctx) {
    int i;
    for (i = 0; i < MAX_ANIM_VEHICLES; i++) {
        AnimVehicle *v = &ctx->anim_vehicles[i];
        if (!v->active) continue;
        float dx = v->tx - v->x;
        float dy = v->ty - v->y;
        float dist = (float)sqrt(dx*dx + dy*dy);
        if (dist < v->speed) { v->active = 0; continue; }
        v->x += (dx / dist) * v->speed;
        v->y += (dy / dist) * v->speed;
        SDL_SetRenderDrawColor(ctx->renderer, 100, 180, 255, 255);
        SDL_Rect rect = {(int)v->x, (int)v->y, 14, 8};
        SDL_RenderFillRect(ctx->renderer, &rect);
    }
}

static void draw_bar_chart(RenderContext *ctx, SharedState *state) {
    int base_x = 20;
    int base_y = 630;
    int bar_w  = 30;
    int gap    = 50;
    int max_h  = 50;

    draw_text(ctx, "Per-lane vehicles passed:", base_x, base_y - 20, WHITE);

    int i;
    for (i = 0; i < NUM_LANES; i++) {
        int count = state->vehicles_per_lane[i];
        int h = (count > 0) ? (count * 3) : 2;
        if (h > max_h) h = max_h;
        SDL_Color col = GREEN_C;
        if (count > 15) col = YELLOW_C;
        if (count > 30) col = RED_C;
        SDL_SetRenderDrawColor(ctx->renderer, col.r, col.g, col.b, 255);
        SDL_Rect bar = {base_x + i * gap, base_y - h, bar_w, h};
        SDL_RenderFillRect(ctx->renderer, &bar);
        char buf[16];
        snprintf(buf, sizeof(buf), "%s", lane_name(i));
        draw_text(ctx, buf, base_x + i * gap - 4, base_y + 4, WHITE);
        snprintf(buf, sizeof(buf), "%d", count);
        draw_text(ctx, buf, base_x + i * gap + 8, base_y - h - 18, WHITE);
    }
}

static void draw_timer(RenderContext *ctx) {
    Uint32 elapsed = (SDL_GetTicks() - ctx->start_ticks) / 1000;
    int h = elapsed / 3600;
    int m = (elapsed % 3600) / 60;
    int s = elapsed % 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "Runtime: %02d:%02d:%02d", h, m, s);
    draw_text(ctx, buf, 700, 20, WHITE);
}

int renderer_init(RenderContext *ctx) {
    memset(ctx, 0, sizeof(RenderContext));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return -1;
    }
    if (TTF_Init() != 0) {
        printf("TTF_Init error: %s\n", TTF_GetError());
        return -1;
    }

    ctx->window = SDL_CreateWindow(
        "Multi-Threaded Traffic Signal Controller",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!ctx->window) { printf("Window: %s\n", SDL_GetError()); return -1; }

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED);
    if (!ctx->renderer) { printf("Renderer: %s\n", SDL_GetError()); return -1; }

    ctx->font = TTF_OpenFont(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13);
    if (!ctx->font)
        ctx->font = TTF_OpenFont(
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 13);
    if (!ctx->font) { printf("Font: %s\n", TTF_GetError()); return -1; }

    ctx->font_large = TTF_OpenFont(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16);
    if (!ctx->font_large) ctx->font_large = ctx->font;

    ctx->start_ticks     = SDL_GetTicks();
    ctx->last_flash_tick = SDL_GetTicks();
    ctx->prev_passed     = 0;

    return 0;
}

void renderer_draw(RenderContext *ctx, SharedState *state) {
    SDL_SetRenderDrawColor(ctx->renderer, 15, 15, 15, 255);
    SDL_RenderClear(ctx->renderer);

    draw_roads(ctx->renderer);

    int positions[NUM_LANES][2] = {
        {408,  80},
        {408, 450},
        { 80, 308},
        {720, 308}
    };

    int cur_passed = state->total_vehicles_passed;
    if (cur_passed > ctx->prev_passed) {
        int lane;
        for (lane = 0; lane < NUM_LANES; lane++) {
            if (state->signal[lane] == GREEN) {
                spawn_anim(ctx, lane, positions);
                break;
            }
        }
        ctx->prev_passed = cur_passed;
    }

    Uint32 now = SDL_GetTicks();
    if (now - ctx->last_flash_tick > 300) {
        ctx->emergency_flash = !ctx->emergency_flash;
        ctx->last_flash_tick = now;
    }

    char buf[64];
    int i;
    for (i = 0; i < NUM_LANES; i++) {
        draw_signal(ctx->renderer,
                    positions[i][0], positions[i][1],
                    state->signal[i]);

        snprintf(buf, sizeof(buf), "%s  Q:%d",
                 lane_name(i), state->queue_count[i]);
        SDL_Color qcol = WHITE;
        if (state->queue_count[i] >= 8)      qcol = RED_C;
        else if (state->queue_count[i] >= 4) qcol = YELLOW_C;
        draw_text(ctx, buf, positions[i][0] - 10, positions[i][1] + 115, qcol);

        int q;
        for (q = 0; q < state->queue_count[i] && q < 6; q++) {
            SDL_SetRenderDrawColor(ctx->renderer, 100, 180, 255, 200);
            SDL_Rect vr;
            if (i == NORTH)
                vr = (SDL_Rect){positions[i][0] + q * 6 - 15, positions[i][1] + 95, 5, 10};
            else if (i == SOUTH)
                vr = (SDL_Rect){positions[i][0] + q * 6 - 15, positions[i][1] + 95, 5, 10};
            else if (i == EAST)
                vr = (SDL_Rect){positions[i][0] + 40, positions[i][1] + q * 6 - 10, 10, 5};
            else
                vr = (SDL_Rect){positions[i][0] - 45, positions[i][1] + q * 6 - 10, 10, 5};
            SDL_RenderFillRect(ctx->renderer, &vr);
        }

        if (state->emergency_active &&
            state->emergency_lane == i &&
            ctx->emergency_flash) {
            draw_text_col(ctx, ctx->font_large,
                          "!! EMERGENCY !!",
                          positions[i][0] - 35,
                          positions[i][1] - 30,
                          RED_C);
        }
    }

    update_anim_vehicles(ctx);

    snprintf(buf, sizeof(buf), "Total passed : %d", state->total_vehicles_passed);
    draw_text(ctx, buf, 20, 20, WHITE);

    snprintf(buf, sizeof(buf), "Throughput   : %.1f v/min", state->throughput_per_min);
    draw_text(ctx, buf, 20, 40, WHITE);

    snprintf(buf, sizeof(buf), "Avg wait     : %.2f s",
             state->total_vehicles_passed > 0
             ? state->total_wait_time_sec / state->total_vehicles_passed
             : 0.0);
    draw_text(ctx, buf, 20, 60, WHITE);

    draw_timer(ctx);
    draw_bar_chart(ctx, state);

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