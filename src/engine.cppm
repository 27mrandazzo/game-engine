module;

#include <limits>
#include <utility>
#include <algorithm>
#include <typeindex>
#include <unordered_map>
#include <cstdlib>
#include <concepts>
#include <cassert>
#include <cstddef>
#include <ranges>
#include <chrono>

export module engine;

import util;

using util::Array;
using std::optional;

template <typename T>
using OptRef = std::optional<std::reference_wrapper<T>>;

#define DEFINE_INDEX_TYPE(Name) \
    struct Name##_T {};         \
    using Name = util::Index<Name##_T>

struct BenchComp { float x, y, z, w; };

constexpr std::size_t WORLD_SIZE = 8192;
constexpr std::size_t MAX_COMPONENTS = 64;

inline std::size_t next_component_id = 0;

template <typename T>
inline std::size_t component_id = next_component_id++;

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

export template <typename T>
struct dup {
    T& first;
    T& second;
    EntityID id_first;
    EntityID id_second;
};

export class SparseSet {
public:
    static auto create(std::size_t c_size) -> SparseSet {
        auto ss = SparseSet(c_size);

        ss.indicies.resize(WORLD_SIZE, dense_sentinel);

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
        if (idx == dense_sentinel) return std::nullopt;

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

    std::vector<std::size_t> indicies;
    util::Vec<EntityID> packed_entities;
    util::raw::Vec packed_components;
};


export class EntityManager {
public:
    EntityManager () {
        stores.reserve(MAX_COMPONENTS);
    }
    auto spawn(this EntityManager& self) -> EntityID {
        EntityID old = EntityID::create(self.n_entities);
        self.n_entities++;
        return old;
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
        auto& set = self.get_store<C>();
        set.remove(entity);
    }

    template <typename First, typename... Rest>
    auto transform(this EntityManager& self, std::invocable<First&, Rest&...> auto f) {
        auto& dense = self.get_store<First>();

        for (int i = 0; i < dense.size(); i++) {
            auto ref = dense[i];
            auto& first = *reinterpret_cast<First*>(ref.ptr());
            auto entity = dense.entity_at(i);
            std::tuple<OptRef<Rest>...> rest = std::tuple(self.get_store<Rest>().template get<Rest>(entity)...);
            
            std::apply([&](OptRef<Rest>&... c) {
                if ((c.has_value() && ...))
                    f(first, c->get()...);
            }, rest);
        }
    }

    template <typename Component>
    auto pairwise_transform(this EntityManager& self, std::invocable<dup<Component>&> auto fn) {
        auto& store = self.get_store<Component>();
        for (auto i = 0; i < store.size(); i++) {
            auto& first = *reinterpret_cast<Component*>(store[i].ptr());
            auto id_first = store.entity_at(i);
            for (int j = i + 1; j < (int)store.size(); j++) {
                auto pair = dup<Component>{ 
                    first,
                    *reinterpret_cast<Component*>(store[j].ptr()),
                    id_first,
                    store.entity_at(j)
                };
                fn(pair);
            }
        }
    }

    auto get_n_entities(this EntityManager& self) -> std::size_t {
        return self.n_entities;
    }

    auto bench(this EntityManager& self) {
        constexpr std::size_t ITERS = 1000000;
        volatile std::size_t* sink = nullptr;

        // ensure BenchComp store exists
        self.get_store<BenchComp>();

        // raw stores[0]
        auto t0 = std::chrono::high_resolution_clock::now();
        for (std::size_t iter = 0; iter < ITERS; iter++) {
            auto& s = self.stores[0];
            sink = reinterpret_cast<volatile std::size_t*>(&s);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double raw_index_ns = std::chrono::duration<double, std::nano>(t1-t0).count() / ITERS;

        // component_id<T> alone
        t0 = std::chrono::high_resolution_clock::now();
        for (std::size_t iter = 0; iter < ITERS; iter++) {
            sink = reinterpret_cast<volatile std::size_t*>(component_id<BenchComp>);
        }
        t1 = std::chrono::high_resolution_clock::now();
        double component_id_ns = std::chrono::duration<double, std::nano>(t1-t0).count() / ITERS;

        // component_id + manual index
        t0 = std::chrono::high_resolution_clock::now();
        for (std::size_t iter = 0; iter < ITERS; iter++) {
            auto id = component_id<BenchComp>;
            auto& s = self.stores[id];
            sink = reinterpret_cast<volatile std::size_t*>(&s);
        }
        t1 = std::chrono::high_resolution_clock::now();
        double manual_ns = std::chrono::duration<double, std::nano>(t1-t0).count() / ITERS;

        // full get_store<T>
        t0 = std::chrono::high_resolution_clock::now();
        for (std::size_t iter = 0; iter < ITERS; iter++) {
            auto& s = self.get_store<BenchComp>();
            sink = reinterpret_cast<volatile std::size_t*>(&s);
        }
        t1 = std::chrono::high_resolution_clock::now();
        double get_store_ns = std::chrono::duration<double, std::nano>(t1-t0).count() / ITERS;

        fprintf(stderr, "=== ENGINE MICROBENCHMARK (ITERS=%zu) ===\n", ITERS);
        fprintf(stderr, "raw stores[0]:          %.2f ns\n", raw_index_ns);
        fprintf(stderr, "component_id<T>:        %.2f ns\n", component_id_ns);
        fprintf(stderr, "component_id + index:   %.2f ns\n", manual_ns);
        fprintf(stderr, "get_store<T>:           %.2f ns\n", get_store_ns);
        fprintf(stderr, "overhead vs raw:        %.2f ns\n", get_store_ns - raw_index_ns);
        fprintf(stderr, "BenchComp id=%zu  stores.size()=%zu  sink=%p\n",
                component_id<BenchComp>, self.stores.size(), (void*)sink);
        fprintf(stderr, "=========================================\n");
        fflush(stderr);
    }

private:
    template <typename T>
    auto get_store(this EntityManager& self) -> SparseSet& {
        const auto id = component_id<T>;

        if (id >= self.stores.size()) {
            assert(id < MAX_COMPONENTS);
            self.stores.resize(id + 1, SparseSet::create(sizeof(T)));
        }

        return self.stores[id];
    }

    std::size_t n_entities = 0;
    std::vector<SparseSet> stores;
};
