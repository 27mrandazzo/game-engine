#include <print>

import engine;

struct Rigidbody { double dx; double dy; };
struct Transform { double x; double y; };
struct RenderMesh {};

auto main() -> int {
    std::print("Hello world!");
    
    auto em = EntityManager();
    em.register_component<Rigidbody>();
    em.register_component<Transform>();
    em.register_component<RenderMesh>();

    auto charlie = EntityID::create(1);
    auto freud = EntityID::create(2);

    em.add<Transform>(charlie, 10.5, 11.5);
    em.add<Rigidbody>(freud, 0.1, 0.3);

    for (int i = 0; i < 10; i++) {
        em.transform<Transform>([](auto& c) {
            c.x += 1;
            c.y -= 1;
            std::println("{} {}", c.x, c.y);
        });
    }

    return 0;
}