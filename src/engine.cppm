module;

#include <limits>
#include <algorithm>
#include <typeindex>
#include <unordered_map>
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

static constexpr std::size_t dense_sentinel = std::numeric_limits<std::size_t>::max();

export class EntityID {
public:
    static constexpr auto create(std::integral auto n) -> EntityID {
        // PRECONDITION
        assert(n < WORLD_SIZE);
        
        return EntityID(n);
    }

    operator std::size_t(this const EntityID& self) {
        return self.value;
    }

    auto operator<=>(const EntityID& other) const = default;
private:
    std::size_t value;
    
    static const std::size_t SENTINEL = std::numeric_limits<std::size_t>::max();

    constexpr EntityID(std::integral auto val) : value(val) {};
};

export class SparseSet {
public:
    static auto create(std::size_t c_size) -> SparseSet {
        auto ss = SparseSet(c_size);

        std::ranges::fill(ss.indicies, dense_sentinel);

        return ss;
    }

    void add(this auto& self, EntityID e, std::invocable<std::byte*> auto cstr) {
        self.indicies[e] = self.packed_entities.size();

        self.packed_entities.emplace_back(e);
        self.packed_components.emplace_back(cstr);
    }

    void remove(this auto& self, EntityID e) {
        self.indicies.at(e) = dense_sentinel;
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
    auto get(this auto& self, EntityID e) -> OptRef<T> {
        auto idx = self.indicies[e];
        if (static_cast<size_t>(e) == dense_sentinel) return std::nullopt;

        util::raw::Ref component = self.packed_components[idx];
        assert(sizeof(T) == component.size());
        return *reinterpret_cast<T*>(component.ptr());
    }

    auto size(this const auto& self) -> std::size_t {
        return self.packed_components.size();
    }
    auto operator[](this auto& self, std::size_t i) -> util::raw::Ref {
        return self.packed_components[i];
    }
    

private:
    SparseSet(std::size_t c_size) : packed_components(c_size) {}

    Array<EntityID, std::size_t, WORLD_SIZE> indicies;
    util::Vec<EntityID> packed_entities;
    util::raw::Vec packed_components;
};

export class EntityManager {
public:
    template<typename C>
    auto register_component(this EntityManager& self) {
        const auto id = std::type_index(typeid(C));
        self.stores.emplace(id, SparseSet::create(sizeof(C)));
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
        auto& dense = self.get_store<First>();

        for (int i = 0; i < dense.size(); i++) {
            auto& first = *reinterpret_cast<First*>(dense[i].ptr());
            auto entity = dense.entity_at(i);
            std::tuple<OptRef<Rest>...> rest = std::tuple(self.get_store<Rest>().template get<Rest>(entity)...);
            
            std::apply([&](OptRef<Rest>&... c) {
                if ((c.has_value() && ...))
                    f(first, c->get()...);
            }, rest);
        }
    }
private:
    template <typename T>
    auto get_store(this EntityManager& self) -> SparseSet& {
        return self.stores.at(std::type_index(typeid(T)));
    }

    
    std::unordered_map<std::type_index, SparseSet> stores;
};
