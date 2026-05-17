#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <cassert>
#include <chrono>
#include <print>
#include <ratio>
#include <thread>
#include <variant>

import engine;
import util;

using namespace std::chrono_literals;

constexpr int w = 500;
constexpr int h = 800;

struct Transform {
  float x;
  float y;
};
struct CollisionMesh {};

struct Rect {
  float w;
  float h;
};
struct Tri {
  float x1, x2, x3, y1, y2, y3;
};
struct RenderMesh {
  SDL_Color color;
  std::variant<Rect, Tri> shape;
};

auto main() -> int {
  std::print("Hello world");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("%s", SDL_GetError());
  }

  auto window = SDL_CreateWindow("Game 1", w, h, 0);
  assert(window != nullptr);

  auto renderer = SDL_CreateRenderer(window, nullptr);
  assert(renderer != nullptr);

  bool running = true;
  uint64_t last = SDL_GetTicks();
  SDL_Event event;

  auto emgr = EntityManager();
  emgr.register_component<Transform>();
  emgr.register_component<CollisionMesh>();

  


  while (running) {
    uint64_t now = SDL_GetTicks();

    emgr.transform<RenderMesh, const Transform>([&renderer](auto rmesh, const auto tf) {
        SDL_SetRenderDrawColor(renderer, 
                            rmesh.color.r, 
                            rmesh.color.g, 
                            rmesh.color.b,
                            rmesh.color.a
        );
        std::visit(util::overloads {
            [&renderer, tf](Rect r) {  
              const auto rect = SDL_FRect { tf.x, tf.y, r.w, r.h };
              SDL_RenderFillRect(renderer, &rect);  
            },
            [&renderer, tf](Tri t) { 
              SDL_RenderLine(renderer, t.x1, t.y1, t.x2, t.y2);
              SDL_RenderLine(renderer, t.x2, t.y2, t.x3, t.y3);
              SDL_RenderLine(renderer, t.x3, t.y3, t.x1, t.y1);
             }
        }, rmesh.shape);
    });

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    if (event.type == SDL_EVENT_QUIT) break;
  }

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);

  SDL_UpdateWindowSurface(window);

  SDL_DestroyRenderer(renderer);

  // SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}