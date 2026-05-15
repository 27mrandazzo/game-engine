#include "SDL3/SDL_error.h"
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
    std::print("Hello world");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("%s", SDL_GetError());
    }

    auto window = SDL_CreateWindow("Game 1", w, h, 0);
    assert(window != nullptr);

    auto renderer = SDL_CreateRenderer(window, nullptr);
    assert(renderer != nullptr);
    
    auto emgr = EntityManager();
    emgr.register_component<Transform>();
    emgr.register_component<CollisionMesh>();

    emgr.add<Transform>(EntityID::create(1), 2.0, 3.0);

    for (int i = 0; i < 1000; i++) {
        emgr.transform<Transform, CollisionMesh>([](auto& t, auto& cm) {
            t.x += 1.0;
            t.y -= 1.0;
            SDL_Log("x: %f y: %f", t.x, t.y);
        });
    }
    

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_UpdateWindowSurface(window);

    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) break;
    }

    SDL_DestroyRenderer(renderer);
    

    // SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    
}