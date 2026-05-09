#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "shared.h"

#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   600
#define REFRESH_MS      50

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
} RenderContext;

int  renderer_init(RenderContext *ctx);
void renderer_draw(RenderContext *ctx, SharedState *state);
void renderer_destroy(RenderContext *ctx);

#endif
