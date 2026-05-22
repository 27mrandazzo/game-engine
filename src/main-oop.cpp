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
#include <vector>

import util;

constexpr int w = 500;
constexpr int h = 800;

auto aabb_axis(std::floating_point auto a_min, std::floating_point auto a_max,
               std::floating_point auto b_min, std::floating_point auto b_max)
    -> bool {
  return a_min <= b_max && b_min <= a_max;
}

struct Rect {
  float w;
  float h;
};
struct Tri {
  float x1, x2, x3, y1, y2, y3;
};

struct Entity {
    bool active = false;
    bool playable = false;
    bool enemy = false;
    bool bullet = false;
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;
    float vx = 0;
    float vy = 0;
    SDL_Color color = {0,0,0,0};
    std::variant<Rect, Tri> shape = Rect{0,0};
    float timer = 0.0f;
    float threshold = 2.0f;
};

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
        bool invincible = false;

        auto entities = std::vector<Entity>(8192); 
        int n_entities = 0;

        auto& player = entities[n_entities++];
        player.active = true;
        player.playable = true;
        player.x = 10; player.y = 20;
        player.w = 15; player.h = 15;
        player.color = SDL_Color{255, 255, 0, 255};
        player.shape = Rect{20, 20};

        while(running) {
            uint64_t now = SDL_GetTicks();
            float dt = (now - last) / 1000.0f;
            last = now;

            const bool *keys = SDL_GetKeyboardState(nullptr);

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT)
                    running = false;
            }

            if (keys[SDL_SCANCODE_I]) invincible = true;

            // input
            for (int i = 0; i < n_entities; i++) {
                auto& e = entities[i];
                if (!e.playable) continue;
                e.vx = 0; e.vy = 0;
                if (keys[SDL_SCANCODE_UP])    e.vy = -200;
                if (keys[SDL_SCANCODE_DOWN])  e.vy =  200;
                if (keys[SDL_SCANCODE_LEFT])  e.vx = -200;
                if (keys[SDL_SCANCODE_RIGHT]) e.vx =  200;
            }

            // collision
            auto before = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < n_entities; i++) {
                if (!entities[i].active) continue;
                for (int j = i + 1; j < n_entities; j++) {
                    if (!entities[j].active) continue;
                    auto& a = entities[i];
                    auto& b = entities[j];
                    bool collision = aabb_axis(a.x, a.x + a.w, b.x, b.x + b.w) &&
                                    aabb_axis(a.y, a.y + a.h, b.y, b.y + b.h);
                    if (collision && a.playable && !invincible) running = false;
                    if (collision && b.playable && !invincible) running = false;
                }
            }
            auto after = std::chrono::high_resolution_clock::now();
            long long n_checks = ((long long)n_entities * (n_entities - 1)) / 2;
            double ns = std::chrono::duration<double, std::nano>(after - before).count();
            double ns_per_check = n_checks > 0 ? ns / n_checks : 0;

            // physics
            for (int i = 0; i < n_entities; i++) {
                auto& e = entities[i];
                if (!e.active) continue;
                e.x += e.vx * dt;
                e.y += e.vy * dt;
            }

            // enemy shoot
            for (int i = 0; i < n_entities; i++) {
                auto& e = entities[i];
                if (!e.enemy) continue;
                e.timer += dt;
                if (e.timer >= e.threshold && n_entities + 4 < 8192) {
                    e.timer = 0;
                    float cx = e.x, cy = e.y;

                    for (auto [vx, vy] : std::initializer_list<std::pair<float,float>>{
                        {0,-100},{0,100},{-100,0},{100,0}
                    }) {
                        auto& b = entities[n_entities++];
                        b.active = true;
                        b.bullet = true;
                        b.x = cx; b.y = cy;
                        b.w = 4;  b.h = 10;
                        b.vx = vx; b.vy = vy;
                        b.color = SDL_Color{255, 100, 0, 255};
                        b.shape = Rect{4, 10};
                    }
                }
            }

            // spawn enemies
            enemy_spawn_timer += dt;
            if (enemy_spawn_timer >= 1.0f && n_entities < 8192) {
                enemy_spawn_timer = 0;
                auto& e = entities[n_entities++];
                e.active = true;
                e.enemy = true;
                e.x = rand() % (w - 50);
                e.y = 0;
                e.w = 8; e.h = 8;
                e.vx = 0; e.vy = 80;
                e.color = SDL_Color{255, 0, 0, 255};
                e.shape = Rect{10, 10};
            }

            // render
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

            if (fps_texture) {
                float tw, th;
                SDL_GetTextureSize(fps_texture, &tw, &th);
                auto rect = SDL_FRect{5, 5, tw, th};
                SDL_RenderTexture(renderer, fps_texture, nullptr, &rect);
            }
            if (entities_texture) {
                float tw, th;
                SDL_GetTextureSize(entities_texture, &tw, &th);
                auto rect = SDL_FRect{5, 35, tw, th};
                SDL_RenderTexture(renderer, entities_texture, nullptr, &rect);
            }
            if (ns_per_check_texture) {
                float tw, th;
                SDL_GetTextureSize(ns_per_check_texture, &tw, &th);
                auto rect = SDL_FRect{5, 65, tw, th};
                SDL_RenderTexture(renderer, ns_per_check_texture, nullptr, &rect);
            }

            for (int i = 0; i < n_entities; i++) {
                auto& e = entities[i];
                if (!e.active) continue;
                SDL_SetRenderDrawColor(renderer, e.color.r, e.color.g, e.color.b, e.color.a);
                std::visit(util::overloads{
                    [&](Rect r) {
                        SDL_FRect rect{e.x, e.y, r.w, r.h};
                        SDL_RenderFillRect(renderer, &rect);
                    },
                    [&](Tri t) {
                        SDL_RenderLine(renderer, e.x+t.x1, e.y+t.y1, e.x+t.x2, e.y+t.y2);
                        SDL_RenderLine(renderer, e.x+t.x2, e.y+t.y2, e.x+t.x3, e.y+t.y3);
                        SDL_RenderLine(renderer, e.x+t.x3, e.y+t.y3, e.x+t.x1, e.y+t.y1);
                    }
                }, e.shape);
            }

            SDL_RenderPresent(renderer);
        }

        if (fps_texture) SDL_DestroyTexture(fps_texture);
        if (entities_texture) SDL_DestroyTexture(entities_texture);
        if (ns_per_check_texture) SDL_DestroyTexture(ns_per_check_texture);



        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    } catch (const std::exception &e) {
    fprintf(stderr, "EXCEPTION: %s\n", e.what());
    fflush(stderr);
  } catch (...) {
    fprintf(stderr, "UNKNOWN EXCEPTION\n");
    fflush(stderr);
  }
}