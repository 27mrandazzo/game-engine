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
using std::optional;

template <typename T>
using OptRef = std::optional<std::reference_wrapper<T>>;

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

    auto entity_at(std::size_t idx) -> EntityID {
        return packed_entities[idx];
    }

    auto query(this auto& self) -> util::raw::Span {
        return self.packed_components.span();
    }

    template<typename T>
    auto get(this auto& self, EntityID e) -> OptRef {
        optional<util::raw::Ref> component = self.packed_components.at(e);

        return optional(component).transform([](auto c){ 
            assert (sizeof(T) == c.size());
            return *reinterpret_cast<T*>(c.ptr());
        });
    }

    auto size(this const auto& self) -> std::size_t {
        return self.packed_components.size();
    }
    auto operator[](this auto& self, std::size_t i) -> util::raw::Ref {
        return self.packed_components[i];
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
        auto& set = self.get_store<C>();
        set.add(entity, [&](std::byte* new_spot) {
            std::construct_at(reinterpret_cast<C*>(new_spot), std::forward<decltype(args)>(args)...);
        });
    }

    template <typename C>
    auto remove(this EntityManager& self, EntityID entity) {
        auto& set = self.stores.at(std::type_index(typeid(C)));
        set.remove(entity);
    }

    template <typename First, typename... Rest>
    auto transform(this EntityManager& self, std::invocable<First&, Rest&...> auto f) {
        auto dense = self.get_store<First>();
        for (int i = 0; i < dense.size(); i++) {
            auto entity = dense.entity_at(i);
            f(*reinterpret_cast<First*>(dense[i].ptr()), self.get<Rest>(entity)...);
        }
    }
private:
    template <typename T>
    auto get_store(this EntityManager& self) -> SparseSet& {
        return self.stores.at(std::type_index(typeid(T)));
    }

    
    std::unordered_map<std::type_index, SparseSet> stores;
};
