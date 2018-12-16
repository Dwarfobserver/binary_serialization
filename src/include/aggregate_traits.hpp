
#pragma once

#include <utility.hpp>

template <class T>
constexpr bool is_aggregate_v = false;

template <class T>
T&& as_tuple(T&& value) {
    return FWD(value);
}
