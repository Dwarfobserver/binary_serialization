
#pragma once

#include <utility>

#define FWD(x) std::forward<decltype(x)>(x)

template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <class T>
struct type_tag {
    using type = T;
};
template <class T>
struct tag_type {
    using type = type_tag<T>;
};

template <auto Value>
struct value_tag {
    static constexpr auto value = Value;
};

// copy (c)ref_t

namespace detail {
    template <class CondT, class T>
    constexpr auto copy_ref_fn() {
        using value_type = std::remove_reference_t<T>;
        using lref_type  = std::conditional_t<std::is_lvalue_reference_v<CondT>, value_type &, value_type>;
        using rref_type  = std::conditional_t<std::is_rvalue_reference_v<CondT>, lref_type &&, lref_type>;
        return type_tag<rref_type>{};
    }
}

template <class CondT, class T>
using copy_ref_t = typename decltype(detail::copy_ref_fn<CondT, T>())::type;

namespace detail {
    template <class CondT, class T>
    constexpr auto copy_cref_fn() {
        using value_type  = remove_cvref_t<T>;
        using value_cond  = std::remove_reference_t<CondT>;
        using result_type = std::conditional_t<std::is_const_v<value_cond>, const value_type, value_type>;
        return copy_ref_fn<CondT, result_type>();
    }
}

template <class CondT, class T>
using copy_cref_t = typename decltype(detail::copy_cref_fn<CondT, T>())::type;

template <class Tuple, template <class> class Map>
struct map_tuple_types;

template <template <class...> class Tuple, class...Ts, template <class> class Map>
struct map_tuple_types<Tuple<Ts...>, Map> {
    using type = Tuple<typename Map<Ts>::type...>;
};

template <class Tuple, template <class> class Map>
using map_tuple_types_t = typename map_tuple_types<Tuple, Map>::type;
