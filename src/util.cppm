module;

#include <array>
#include <vector>
#include <optional>
#include <functional>
#include <compare>

export module util;

export namespace util {
    template <typename Tag>
    struct Index {
        std::size_t value;
        auto operator<=>(const Index&) const = default;
    };

    template <typename T>
    concept IndexType = std::three_way_comparable<T> && std::copyable<T>;

    template <IndexType I, typename T, std::size_t N>
    class Array {
    public:
        constexpr auto size() const noexcept -> std::size_t {
            return N;
        }

        constexpr auto operator[](this auto& self, I idx) -> auto& {
            return self.arr[idx.value];
        }

        constexpr auto at(this auto& self, I idx) {
            using Ref = decltype(std::ref(self.arr[0]));
            if (idx.value >= N) return std::optional<Ref>{std::nullopt};
            return std::optional<Ref>{std::ref(self.arr[idx.value])};
        }

        constexpr auto update(I idx, std::invocable<T&> auto fn) -> bool {
            if (idx.value >= N) return false;
            fn(arr[idx.value]);
            return true;
        }

        constexpr auto begin(this auto& self) noexcept { return self.arr.begin(); }
        constexpr auto end(this auto& self) noexcept { return self.arr.end(); }

    private:
        std::array<T, N> arr;
    };

    template <IndexType I>
    class Vector {
    public:
        Vector() = default;

        explicit Vector(std::size_t size) : arr(size) {}

        Vector(std::initializer_list<T> init) : arr(init) {}

        auto size() const noexcept -> std::size_t {
            return arr.size();
        }

        auto empty() const noexcept -> bool {
            return arr.empty();
        }

        auto capacity() const noexcept -> std::size_t {
            return arr.capacity();
        }

        auto reserve(std::size_t n) -> void {
            arr.reserve(n);
        }

        auto operator[](this auto& self, I idx) -> auto& {
            return self.arr[idx.value];
        }

        auto at(this auto& self, I idx) {
            using Ref = decltype(std::ref(self.arr[0]));
            if (idx.value >= self.arr.size()) return std::optional<Ref>{std::nullopt};
            return std::optional<Ref>{std::ref(self.arr[idx.value])};
        }

        auto update(I idx, std::invocable<T&> auto fn) -> bool {
            if (idx.value >= arr.size()) return false;
            fn(arr[idx.value]);
            return true;
        }

        auto push_back(T val) -> void {
            arr.push_back(std::move(val));
        }

        auto pop_back() -> std::optional<T> {
            if (arr.empty()) return std::nullopt;
            T val = std::move(arr.back());
            arr.pop_back();
            return val;
        }

        auto begin(this auto& self) noexcept { return self.arr.begin(); }
        auto end(this auto& self) noexcept { return self.arr.end(); }

    private:
        std::vector<T> arr;
    };
}