module;

#include <cstdint>
#include <cstdlib>
#include <concepts>
#include <limits>

export module engine;

import util;

using util::Vector;
using util::Array;

export template <std::size_t WORLD_SIZE = 1024>
class EntityID {
    
    static constexpr std::size_t SIZE_T_MAX = std::numeric_limits<std::size_t>::max();
    static_assert(WORLD_SIZE != SIZE_T_MAX, "SIZE_T_MAX must be left for sentinel value");
    static constexpr std::size_t SENTINEL = SIZE_T_MAX; 

    std::size_t value;

    constexpr EntityID(std::integral auto val) {
        value = val;
    }

    explicit constexpr EntityID(std::uint32_t n) : value(n) {};
};

/// RTTI
export struct ComponentID {
    std::size_t typeIdx;

    explicit constexpr ComponentID(std::uint32_t typeIdx) : typeIdx(typeIdx) {};
};

template <util::IndexType I, typename T>
class PackedList {
public:
    explicit PackedList() {}

    auto push(this auto& self, T val) -> void {
        self.data.push_back(val);
    }
    auto pop(this auto& self, I idx) -> void {
        if (idx >= self.data.size() || idx == self.data.size() - 1) {
            return;
        }

        // Remove item, and put last into that spot, to keep list compact.
        T end = self.data.pop_back().value();
        self.data.at(idx) = end;
    }
    auto for_each(this auto& self, std::invocable<T> auto f) {
        for (T& item : self.data) f();
    };

private:
    util::Vector<I, T> data;
};


template <std::size_t UNIVERSE_SIZE>
class SparseSet {
public:
    explicit SparseSet() {}

private:
    struct PackedIdx_T {};
    using PackedIdx = util::Index<PackedIdx_T>;

    Array<EntityID<>, PackedIdx, UNIVERSE_SIZE> indicies;
    PackedList<PackedIdx, EntityID<>> packed_entities;
    PackedList<PackedIdx, std::byte> packed_components;
};

