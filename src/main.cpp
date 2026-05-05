#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"
#include <cassert>
#include <print>
#include <SDL3/SDL.h>
#include <ratio>
#include <thread>
#include <chrono>

import engine;

using namespace std::chrono_literals;

constexpr int w = 500;
constexpr int h = 800;

struct Transform { double x; double y; };
struct CollisionMesh { };

auto main() -> int {
    std::print("Hello world!");
    
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO));

    auto window = SDL_CreateWindow("Game 1", w, h, 0);
    auto renderer = SDL_CreateRenderer(window, nullptr);

    SDL_Log("%s", SDL_GetError());
    assert(window != nullptr);
    assert(renderer != nullptr);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_UpdateWindowSurface(window);

    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) break;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    
}