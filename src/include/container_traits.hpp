
// range
// -> container
// -> array
//    -> static_array
//    -> dynamic_array

#pragma once

#include <utility.hpp>
#include <iterator>
#include <tuple>

// has_fn() -> is_invocable(fn(args...) | std::fn(args...))
#define MAKE_STD_WRAP(fn) \
    template <class...Args> \
    constexpr bool has_##fn() noexcept { \
        using std::fn; \
        constexpr auto expr = [] (auto&&...args) -> decltype( \
            fn(FWD(args)...) \
        ) {}; \
        return std::is_invocable_v<decltype(expr), Args...>; \
    } \
    template <class...Args> \
    constexpr decltype(auto) call_##fn(Args&&...args) noexcept { \
        using std::fn; \
        return fn(FWD(args)...); \
    }

// Range

namespace detail::no_adl {
    MAKE_STD_WRAP(begin)
    MAKE_STD_WRAP(end)
    MAKE_STD_WRAP(size)

    template <class T, class SFINAE = void>
    struct value_type_or_void {
        using type = void;
    };
    template <class T>
    struct value_type_or_void<T, std::enable_if_t<
        has_begin<T>()
    >> {
        using type = std::decay_t<decltype(*call_begin(std::declval<T&&>()))>;
    };
    
    template <class T>
    using value_type_t = typename value_type_or_void<T>::type;
    template <class T>
    constexpr bool has_value_type_v = !std::is_same_v<value_type_t<T>, void>;
}

template <class T>
constexpr bool is_range_v = detail::no_adl::has_begin<T>()
                         && detail::no_adl::has_end<T>()
                         && detail::no_adl::has_size<T>()
                         && detail::no_adl::has_value_type_v<T>;

template <class Range>
struct range_traits {
    static_assert(is_range_v<Range>);

    using value_type = detail::no_adl::value_type_t<Range>;

    static constexpr decltype(auto) begin(Range& range)       noexcept { return detail::no_adl::call_begin(range); }
    static constexpr decltype(auto) begin(Range const& range) noexcept { return detail::no_adl::call_begin(range); }
    static constexpr decltype(auto) end(Range& range)         noexcept { return detail::no_adl::call_end(range); }
    static constexpr decltype(auto) end(Range const& range)   noexcept { return detail::no_adl::call_end(range); }
    static constexpr decltype(auto) size(Range const& range)  noexcept { return detail::no_adl::call_size(range); }
};

// Container

namespace detail::no_adl {
    template <class T>
    constexpr bool has_emplace() noexcept {
        constexpr auto emplace_back_expr = [] (auto& arg) -> decltype(
            arg.emplace_back()
        ) {};
        constexpr auto emplace_front_expr = [] (auto& arg) -> decltype(
            arg.emplace_front()
        ) {};
        constexpr auto emplace_expr = [] (auto& arg) -> decltype(
            arg.emplace()
        ) {};

        return std::is_invocable_v<decltype(emplace_back_expr), T&>
            || std::is_invocable_v<decltype(emplace_front_expr), T&> 
            || std::is_invocable_v<decltype(emplace_expr), T&>;
    }

    template <class T>
    constexpr decltype(auto) call_emplace(T& range) {
        constexpr auto emplace_back_expr = [] (auto& arg) -> decltype(
            arg.emplace_back()
        ) {};
        constexpr auto emplace_front_expr = [] (auto& arg) -> decltype(
            arg.emplace_front()
        ) {};

        if constexpr (std::is_invocable_v<decltype(emplace_back_expr), T&>) {
            return range.emplace_back();
        }
        else if constexpr (std::is_invocable_v<decltype(emplace_front_expr), T&>) {
            return range.emplace_front();
        }
        else {
            return range.emplace();
        }
    }

    template <class T>
    void try_reserve(T& range) {
        constexpr auto expr = [] (auto& arg) -> decltype(
            arg.reserve()
        ) {};
        if constexpr (std::is_invocable_v<decltype(expr), T&>) {
            range.reserve();
        }
    }
}

template <class T>
constexpr auto is_container_v = is_range_v<T> && detail::no_adl::has_emplace<T>();

template <class Container>
struct container_traits : range_traits<Container> {
    static_assert(is_container_v<Container>);

    static constexpr decltype(auto) emplace(Container& c) { return detail::no_adl::call_emplace(c); }
    static constexpr void try_reserve(Container& c) { detail::no_adl::try_reserve(c); }
};

// Array

namespace detail::no_adl {
    MAKE_STD_WRAP(data)
}

template <class T>
constexpr bool is_array_v = is_range_v<T> && detail::no_adl::has_data<T>();

template <class Array>
struct array_traits : range_traits<Array> {
    static_assert(is_array_v<Array>);

    static constexpr decltype(auto) data(Array& array)       noexcept { return detail::no_adl::call_data(array); }
    static constexpr decltype(auto) data(Array const& array) noexcept { return detail::no_adl::call_data(array); }
};

// DynamicArray

namespace detail::no_adl {
    template <class DynamicArray>
    constexpr bool has_resize() noexcept {
        constexpr auto expr = [] (auto&& arr) -> decltype(
            arr.resize(size_t{})
        ) {};
        return std::is_invocable_v<decltype(expr), DynamicArray>;
    }
}

template <class T>
constexpr bool is_dynamic_array_v = is_array_v<T> && detail::no_adl::has_resize<T>();

template <class Array>
struct dynamic_array_traits : array_traits<Array> {
    static_assert(is_dynamic_array_v<Array>);

    static constexpr void resize(Array& array, size_t size) { array.resize(size); }
};

// FixedSize

namespace detail::no_adl {
    template <class T>
    constexpr bool has_fixed_size() noexcept {
        constexpr auto expr = [] (auto&& tuple) ->
            std::tuple_size<remove_cvref_t<decltype(tuple)>>
        {};
        return std::is_invocable_v<decltype(expr), T>;
    }
}

template <class T>
constexpr bool has_fixed_size_v = detail::no_adl::has_fixed_size<T>();

template <class T>
constexpr size_t get_fixed_size() noexcept {
    return std::tuple_size_v<remove_cvref_t<T>>;
}
