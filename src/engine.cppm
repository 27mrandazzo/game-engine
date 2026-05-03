module;

#include <algorithm>
#include <typeindex>
#include <unordered_map>
#include <cstdint>
#include <cstdlib>
#include <concepts>
#include <cassert>
#include <cstddef>

export module engine;

import util;

using util::Array;

#define DEFINE_INDEX_TYPE(Name) \
    struct Name##_T {};         \
    using Name = util::Index<Name##_T>

constexpr std::size_t WORLD_SIZE = 1024;

export class EntityID {
public:
    static constexpr auto create(std::integral auto n) -> EntityID {
        // PRECONDITION
        assert(n != SENTINEL);
        assert(n < WORLD_SIZE);
        
        return EntityID(n);
    }
    static constexpr auto sentinel() -> EntityID {
        return EntityID(SENTINEL);
    }

    operator std::size_t(this const EntityID& self) {
        return self.value;
    }

    auto operator<=>(const EntityID& other) const = default;
private:
    std::size_t value;
    
    static const std::size_t SENTINEL = 0;

    constexpr EntityID(std::integral auto val) : value(val) {};
};

export class SparseSet {
public:
    explicit SparseSet(std::size_t c_size) : packed_components(c_size) {}

    void add(this auto& self, EntityID e, std::invocable<std::byte*> auto cstr) {
        assert(e != EntityID::sentinel());

        self.indicies[e] = self.packed_entities.size();

        self.packed_entities.emplace_back(e);
        self.packed_components.emplace_back(cstr);
    }

    void remove(this auto& self, EntityID e) {
        self.indicies.at(e) = EntityID::sentinel();
        size_t last_idx = self.packed_entities.size() - 1;
        self.packed_entities.pop_swap(last_idx);
        self.packed_components.pop_swap(last_idx);
    }

    auto query(this auto& self) -> util::raw::Span {
        return self.packed_components.span();
    }
    

private:
    Array<EntityID, std::size_t, WORLD_SIZE> indicies;
    util::Vec<EntityID> packed_entities;
    util::raw::Vec packed_components;
};

export class EntityManager {
public:
    template<typename C>
    auto register_component(this EntityManager& self) {
        const auto id = std::type_index(typeid(C));
        self.stores.emplace(id, SparseSet(sizeof(C)));
    }

    template <typename C>
    auto add(this EntityManager& self, EntityID entity, auto&&... args) {
        auto& set = self.stores.at(std::type_index(typeid(C)));
        set.add(entity, [&](std::byte* new_spot) {
            std::construct_at(reinterpret_cast<C*>(new_spot), std::forward<decltype(args)>(args)...);
        });
    }

    template <typename C>
    auto remove(this EntityManager& self, EntityID entity) {
        auto& set = self.stores.at(std::type_index(typeid(C)));
        set.remove(entity);
    }

    template <typename C>
    auto transform(this EntityManager& self, std::invocable<C&> auto f) {
        auto items = self.stores.at(std::type_index(typeid(C))).query();
        for (auto item : items) {
            f(*reinterpret_cast<C*>(item.ptr()));
        }
    }
private:
    std::unordered_map<std::type_index, SparseSet> stores;
};
