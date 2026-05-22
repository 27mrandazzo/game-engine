#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cassert>
#include <chrono>
#include <concepts>
#include <print>
#include <ratio>
#include <stdexcept>
#include <variant>

import engine;
import util;

using namespace std::chrono_literals;

constexpr int w = 500;
constexpr int h = 800;

constexpr double speed = 200;
constexpr double bullet_speed = 100;

constexpr double bulletLen = 10;
constexpr double bulletWid = 4;

auto aabb_axis(std::floating_point auto a_min, std::floating_point auto a_max,
               std::floating_point auto b_min, std::floating_point auto b_max)
    -> bool {
  return a_min <= b_max && b_min <= a_max;
}

struct Transform {
  float x, y, w, h;
};

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
struct Playable {};
struct Collided {
  EntityID other;
};
struct Rigidbody {
  float vx, vy;
};
struct Enemy {
  float timer = 0.0f;
  float threshold = 2.0f;
};
struct Bullet {};

auto main() -> int {
  try {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      SDL_Log("%s", SDL_GetError());
    }

    auto window = SDL_CreateWindow("Game 1", w, h, 0);
    assert(window != nullptr);

    auto renderer = SDL_CreateRenderer(window, "software");
    assert(renderer != nullptr);

    bool running = true;
    uint64_t last = SDL_GetTicks();
    SDL_Event event;

    if (!TTF_Init()) throw std::runtime_error("Cannot init TTF font library");
    const char *font_path = SDL_getenv("FONT_PATH");
    TTF_Font *font = TTF_OpenFont(font_path, 24);
    assert(font != nullptr);

    SDL_Texture* fps_texture = nullptr;
    SDL_Texture* entities_texture = nullptr;
    SDL_Texture* ns_per_check_texture = nullptr;
    int last_fps_display = -1;
    int last_n_entities = -1;

    float enemy_spawn_timer = 0;

    auto emgr = EntityManager();
    emgr.bench();

    auto player = emgr.spawn();
    emgr.add<RenderMesh>(player, SDL_Color{255, 255, 0, 1}, Rect{20, 20});
    emgr.add<Transform>(player, 10, 20, 15, 15);
    emgr.add<Playable>(player);
    emgr.add<Rigidbody>(player);
    bool invincible = false;

    while (running) {
      uint64_t now = SDL_GetTicks();
      float dt = (now - last) / 1000.0f;
      last = now;

      const bool *keys = SDL_GetKeyboardState(nullptr);

      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)
          running = false;
      }

      if (keys[SDL_SCANCODE_I]) invincible = true;

      emgr.transform<Playable, Rigidbody>(
          [keys](Playable &_, Rigidbody &rb) {
            rb.vx = 0;
            rb.vy = 0;
            if (keys[SDL_SCANCODE_UP])
              rb.vy = -speed;
            if (keys[SDL_SCANCODE_DOWN])
              rb.vy = speed;
            if (keys[SDL_SCANCODE_LEFT])
              rb.vx = -speed;
            if (keys[SDL_SCANCODE_RIGHT])
              rb.vx = speed;
          });

      // Collision
      auto n_entities = emgr.get_n_entities();
      auto before = std::chrono::high_resolution_clock::now();

      emgr.pairwise_transform<Transform>([&emgr](dup<Transform> &pair) {
        float x1_min = pair.first.x;
        float x2_min = pair.second.x;
        float x1_max = pair.first.x + pair.first.w;
        float x2_max = pair.second.x + pair.second.w;

        float y1_min = pair.first.y;
        float y2_min = pair.second.y;
        float y1_max = pair.first.y + pair.first.h;
        float y2_max = pair.second.y + pair.second.h;

        bool collision = aabb_axis(x1_min, x1_max, x2_min, x2_max) &&
                         aabb_axis(y1_min, y1_max, y2_min, y2_max);

        if (collision) {
          emgr.add<Collided>(pair.id_first, pair.id_second);
          emgr.add<Collided>(pair.id_second, pair.id_first);
        }
      });

      auto after = std::chrono::high_resolution_clock::now();
      auto duration = after - before;
      std::integral auto n_checks = (n_entities * (n_entities - 1)) / 2;
      auto ns = std::chrono::duration<float, std::nano>(duration).count();
      double ns_per_check = n_checks > 0 ? ns / n_checks : 0;

      // Physics
      emgr.transform<Transform, Rigidbody>(
          [dt](auto &tf, auto &rb) {
            tf.x += rb.vx * dt;
            tf.y += rb.vy * dt;
          });

      emgr.transform<Playable, Collided>(
          [&running, invincible](auto &_, auto &__) { if (!invincible) running = false;  });

      enemy_spawn_timer += dt;
      if (enemy_spawn_timer >= 1.0f) {
        enemy_spawn_timer = 0;

        auto e = emgr.spawn();
        float x = rand() % (w - 50); // random x position
        emgr.add<Transform>(e, x, 0, 8, 8);
        emgr.add<RenderMesh>(e, SDL_Color{255, 0, 0, 255}, Rect{10, 10});
        emgr.add<Enemy>(e);
        emgr.add<Rigidbody>(e, 0, 80);
      }

      emgr.transform<Enemy, Transform, Rigidbody>(
          [dt, &emgr](auto &enemy, auto &tf, auto &rb) {
            enemy.timer += dt;
            if (enemy.timer >= enemy.threshold) {
              enemy.timer = 0;
              auto bulletUp = emgr.spawn();
              auto bulletDown = emgr.spawn();
              auto bulletLeft = emgr.spawn();
              auto bulletRight = emgr.spawn();

              emgr.add<Transform>(bulletUp, tf.x, tf.y, bulletWid, bulletLen);
              emgr.add<Transform>(bulletDown, tf.x, tf.y, bulletWid, bulletLen);
              emgr.add<Transform>(bulletLeft, tf.x, tf.y, bulletWid, bulletLen);
              emgr.add<Transform>(bulletRight, tf.x, tf.y, bulletWid,
                                  bulletLen);

              emgr.add<Rigidbody>(bulletUp, 0, -bullet_speed);
              emgr.add<Rigidbody>(bulletDown, 0, bullet_speed);
              emgr.add<Rigidbody>(bulletLeft, -bullet_speed, 0);
              emgr.add<Rigidbody>(bulletRight, bullet_speed, 0);

              emgr.add<Bullet>(bulletUp);
              emgr.add<Bullet>(bulletDown);
              emgr.add<Bullet>(bulletLeft);
              emgr.add<Bullet>(bulletRight);

              emgr.add<RenderMesh>(bulletUp,    SDL_Color{255, 100, 0, 255},
                  Tri{-bulletWid/2, bulletWid/2, 0,  bulletLen, bulletLen, 0});
              emgr.add<RenderMesh>(bulletDown,  SDL_Color{255, 100, 0, 255},
                  Tri{-bulletWid/2, bulletWid/2, 0,  0, 0, bulletLen});
              emgr.add<RenderMesh>(bulletLeft, SDL_Color{255, 100, 0, 255},
                  Tri{0, -bulletLen, 0,  -bulletWid/2, 0, bulletWid/2});
              emgr.add<RenderMesh>(bulletRight, SDL_Color{255, 100, 0, 255},
                  Tri{0, bulletLen, 0,  -bulletWid/2, 0, bulletWid/2});
            }
          });

      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);

      static float fps_timer = 0;
      static int fps_display = 0;
      fps_timer += dt;
      if (fps_timer >= 1.0f) {
        fps_display = (int)(1.0f / dt);
        fps_timer = 0;

        if (ns_per_check_texture) SDL_DestroyTexture(ns_per_check_texture);
        std::string ns_str = "ns/check: " + std::to_string(ns_per_check);
        SDL_Surface* s = TTF_RenderText_Solid(font, ns_str.c_str(), ns_str.size(), SDL_Color{255,255,255,255});
        if (s) {
            SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
            SDL_DestroySurface(s);
            if (t) ns_per_check_texture = t;
        }
      }

      n_entities = emgr.get_n_entities();

      if (fps_display != last_fps_display) {
          if (fps_texture) SDL_DestroyTexture(fps_texture);
          std::string fps_str = "FPS: " + std::to_string(fps_display);
          SDL_Surface* s = TTF_RenderText_Solid(font, fps_str.c_str(), fps_str.size(), SDL_Color{255,255,255,255});
          if (s) { fps_texture = SDL_CreateTextureFromSurface(renderer, s); SDL_DestroySurface(s); }
          last_fps_display = fps_display;
      }

      if (n_entities != last_n_entities) {
          if (entities_texture) SDL_DestroyTexture(entities_texture);
          std::string e_str = "Entities: " + std::to_string(n_entities);
          SDL_Surface* s = TTF_RenderText_Solid(font, e_str.c_str(), e_str.size(), SDL_Color{255,255,255,255});
          if (s) { entities_texture = SDL_CreateTextureFromSurface(renderer, s); SDL_DestroySurface(s); }
          last_n_entities = n_entities;
      }

      // render all
      if (fps_texture) {
          float tw, th;
          SDL_GetTextureSize(fps_texture, &tw, &th);
          SDL_FRect rect = {5, 5, tw, th};
          SDL_RenderTexture(renderer, fps_texture, nullptr, &rect);
      }
      if (entities_texture) {
          float tw, th;
          SDL_GetTextureSize(entities_texture, &tw, &th);
          SDL_FRect rect = {5, 35, tw, th};
          SDL_RenderTexture(renderer, entities_texture, nullptr, &rect);
      }
      if (ns_per_check_texture) {
        float tw, th;
        SDL_GetTextureSize(ns_per_check_texture, &tw, &th);
        SDL_FRect rect = {5, 65, tw, th};
        SDL_RenderTexture(renderer, ns_per_check_texture, nullptr, &rect);
      }

      emgr.transform<RenderMesh, Transform>([&renderer](auto rmesh,
                                                              const auto tf) {
        SDL_SetRenderDrawColor(renderer, rmesh.color.r, rmesh.color.g,
                               rmesh.color.b, rmesh.color.a);
        std::visit(util::overloads{
                       [&renderer, tf](Rect r) {
                         const auto rect = SDL_FRect{tf.x, tf.y, r.w, r.h};
                         SDL_RenderFillRect(renderer, &rect);
                       },
                       [&renderer, tf](Tri tri) {
                         SDL_RenderLine(renderer, tf.x + tri.x1, tf.y + tri.y1,
                                        tf.x + tri.x2, tf.y + tri.y2);
                         SDL_RenderLine(renderer, tf.x + tri.x2, tf.y + tri.y2,
                                        tf.x + tri.x3, tf.y + tri.y3);
                         SDL_RenderLine(renderer, tf.x + tri.x3, tf.y + tri.y3,
                                        tf.x + tri.x1, tf.y + tri.y1);
                       }},
                   rmesh.shape);
      });

      SDL_RenderPresent(renderer);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_UpdateWindowSurface(window);

    SDL_DestroyRenderer(renderer);



    // SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
  } catch (const std::exception &e) {
    fprintf(stderr, "EXCEPTION: %s\n", e.what());
    fflush(stderr);
  } catch (...) {
    fprintf(stderr, "UNKNOWN EXCEPTION\n");
    fflush(stderr);
  }
}
