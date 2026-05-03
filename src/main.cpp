#include <print>

import engine;

auto main() -> int {
    std::print("Hello world!");
    
    auto ss = SparseSet(sizeof(std::string));

    ss.add(EntityID::create(3), [](std::byte* b) { 
        std::construct_at(reinterpret_cast<std::string*>(b), "hello world!"); 
    });

    for (auto ref : ss.query()) {
        std::println("{}", *reinterpret_cast<std::string*>(ref.ptr()));
    }

    return 0;
}