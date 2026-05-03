module;

#include <array>
#include <vector>
#include <optional>
#include <functional>
#include <compare>
#include <span>
#include <cstddef>
#include <concepts>

export module util;

#define DEFINE_INDEX_TYPE(Name) \
    struct Name##_T {};         \
    using Name = util::Index<Name##_T>

export namespace util {
    template <typename Tag>
    struct Index {
        std::size_t value;
        auto operator<=>(const Index&) const = default;

        operator size_t() {
            return value;
        }
    };

    template <typename T>
    concept IndexType = std::three_way_comparable<T> && std::copyable<T> && std::convertible_to<T, std::size_t>;

    template <IndexType I, typename T, std::size_t N>
    class Array {
    public:
        constexpr auto size() const noexcept -> std::size_t {
            return N;
        }

        constexpr auto operator[](this auto& self, I idx) -> auto& {
            return self.arr[idx];
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

    template <IndexType I, typename T>
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

    namespace raw {
        class Ref {
        public:
            Ref(std::byte* ptr, std::size_t size) : data(ptr, size) {};

            auto ptr(this const auto& self) -> std::byte* {
                return self.data.data();
            }
            auto size(this const Ref& self) -> std::size_t {
                return self.data.size();
            }
            auto copy_from(this auto& self, Ref other) {
                std::copy(other.data.begin(), other.data.end(), self.data.begin());
            }
        private:
            std::span<std::byte> data;
        };

        class Span {
        public:
            Span(std::byte* ptr, std::size_t stride, std::size_t len) : ptr(ptr, stride * len), stride(stride) {};
            Span() = delete;

            struct Iterator {
                std::byte* ptr;
                std::size_t stride;

                auto operator*() const -> Ref { return Ref(ptr, stride); }
                auto operator++() -> Iterator& { ptr += stride; return *this; }
                auto operator==(const Iterator& other) const -> bool { return ptr == other.ptr; }
            };

            auto begin() -> Iterator { return { ptr.data(), stride }; }
            auto end() -> Iterator { return { ptr.data() + ptr.size(), stride }; }

            
        private:
            std::span<std::byte> ptr;
            const std::size_t stride;
        };

        class Vec {
        public:
            Vec(std::size_t size) : stride(size) {};
            Vec() = delete;

            auto emplace_back(this auto& self, std::invocable<std::byte*> auto cstr) {
                std::size_t offset = self.bytes.size();
                self.bytes.resize(self.bytes.size() + self.stride);
                cstr(self.bytes.data() + offset);
            }
            auto pop_swap(this auto& self, std::size_t idx) -> void {
                self[idx].copy_from(self[self.size() - 1]);
                self.bytes.resize(self.bytes.size() - self.stride);
            }
            auto size(this const auto& self) -> std::size_t {
                return self.bytes.size() / self.stride;
            }
            auto operator[](this const auto& self, std::size_t idx) -> Ref {
                std::byte* item = self.bytes.data() + (idx * self.stride);
                return Ref(item, self.stride);
            }
            auto at(this const auto& self, std::size_t idx) -> std::optional<Ref> {
                if (idx >= self.size()) return std::nullopt;
                return self[idx];
            }
            auto span(this auto& self) -> Span {
                return Span(self.bytes.data(), self.stride, self.size());
            }
        private:
            std::vector<std::byte> bytes;
            const std::size_t stride;
        };
    }

    template <typename T>
    class Vec {
    public:
        Vec() : data(sizeof(T)) {};

        auto operator[](this const auto& self, std::size_t idx) -> T& {
            raw::Ref bytes = self.data[idx];
            return *reinterpret_cast<T*>(bytes.ptr());
        }

        auto at(this const auto& self, std::size_t idx) -> std::optional<std::reference_wrapper<T>> {
            if (idx >= self.data.size()) return std::nullopt;
            return self[idx];
        }

        auto emplace_back(this auto& self, auto&&... args) 
            requires std::constructible_from<T, decltype(args)...>
        {
            self.data.emplace_back([&](std::byte* slot) {
                std::construct_at(reinterpret_cast<T*>(slot), std::forward<decltype(args)>(args)...);
            });
        }

        auto size(this const auto& self) -> std::size_t {
            return self.data.size();
        }

        auto pop_swap(this auto& self, std::size_t idx) {
            self.data.pop_swap(idx);
        }
    private:
        raw::Vec data;
    };


}