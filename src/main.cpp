#include <print>

import util;

auto main() -> int {
    std::print("Hello world!");
    
    struct T {};
    using Idx = util::Index<T>;
    util::Vector vec = util::Vector<int, Idx>({2, 3, 5});
    vec.update(Idx(3), [](int x) { x++; });

    return 0;
}