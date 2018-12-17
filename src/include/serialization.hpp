
#pragma once

#include <aggregate_traits.hpp>
#include <span.hpp>
#include <algorithm>
#include <array>
#include <cstring>

enum class serialization_category {
    forbidden,
    trivial,
    trivial_array,
    fixed_array,
    dynamic_array,
    container,
    tuple,
    aggregate,
    unknown,
};

namespace detail::no_adl {
    template <class T>
    constexpr bool has_std_get() noexcept {
        constexpr auto expr = [] (auto&& val) -> decltype(
            std::get<0>(val)
        ) {};
        return std::is_invocable_v<decltype(expr), T>;
    }
}

namespace detail {
    template <class T>
    constexpr serialization_category get_serialization_category() {
        if constexpr (std::is_pointer_v<T> || std::is_array_v<T>) {
            return serialization_category::forbidden;
        }
        else if constexpr (std::is_trivially_copyable_v<T>) {
            return serialization_category::trivial;
        }
        else if constexpr (is_array_v<T>) {
            if constexpr (std::is_trivially_copyable_v<typename array_traits<T>::value_type>) {
                static_assert(is_dynamic_array_v<T>);
                return serialization_category::trivial_array;
            }
            else if constexpr (is_dynamic_array_v<T>) {
                return serialization_category::dynamic_array;
            }
            else {
                static_assert(has_fixed_size_v<T>);
                return serialization_category::fixed_array;
            }
        }
        else if constexpr (is_container_v<T>) {
            return serialization_category::container;
        }
        else if constexpr (detail::no_adl::has_std_get<T>() && detail::no_adl::has_fixed_size<T>()) {
            return serialization_category::tuple;
        }
        else if constexpr (is_aggregate_v<T>) {
            return serialization_category::aggregate;
        }
        else {
            return serialization_category::unknown;
        }
    }
}

template <class T>
constexpr auto serialization_category_v = detail::get_serialization_category<T>();
template <class T>
using serialization_category_tag_t = value_tag<serialization_category_v<T>>;

// functions

template <class T>
void serialize(T const& value, span<std::byte>& buffer) noexcept;

template <class T>
void deserialize(T& value, span<std::byte const>& buffer);

template <class T>
constexpr size_t get_serialized_size(T const& value) noexcept;

template <class T>
std::pair<size_t, bool> try_get_deserialized_size(span<std::byte const> buffer) noexcept;

// serialize

namespace detail {
    template <class T, serialization_category Category>
    void do_serialize(T const&, span<std::byte>&, value_tag<Category>) noexcept {
        static_assert(serialization_category_v<T> != serialization_category::forbidden);
        static_assert(serialization_category_v<T> != serialization_category::unknown);
    }
    template <class T>
    void do_serialize(T const& value, span<std::byte>& buffer, value_tag<serialization_category::trivial>) noexcept {
        memcpy(buffer.data(), &value, sizeof(T));
        buffer.begin() += sizeof(T);
    }
    template <class T>
    void do_serialize(T const& array, span<std::byte>& buffer, value_tag<serialization_category::trivial_array>) noexcept {
        using traits = array_traits<T>;
        serialize(traits::size(array), buffer);

        auto const elements_size = traits::size(array) * sizeof(typename traits::value_type);
        memcpy(buffer.data(), traits::data(array), elements_size);
        buffer.begin() += elements_size;
    }

    template <class T>
    void serialize_range(T const& range, span<std::byte>& buffer) noexcept {
        using traits = range_traits<T>;
        std::for_each(traits::begin(range), traits::end(range), [&] (auto& val) {
            serialize(val, buffer);
        });
    }

    template <class T>
    void do_serialize(T const& container, span<std::byte>& buffer, value_tag<serialization_category::container>) noexcept {
        size_t const count = range_traits<T>::size(container);
        serialize(count, buffer);
        serialize_range(container, buffer);
    }
    template <class T>
    void do_serialize(T const& array, span<std::byte>& buffer, value_tag<serialization_category::fixed_array>) noexcept {
        serialize_range(array, buffer);
    }
    template <class T>
    void do_serialize(T const& array, span<std::byte>& buffer, value_tag<serialization_category::dynamic_array>) noexcept {
        size_t const count = range_traits<T>::size(array);
        serialize(count, buffer);
        serialize_range(array, buffer);
    }
    template <class T>
    void do_serialize(T const& value, span<std::byte>& buffer, value_tag<serialization_category::tuple>) noexcept {
        std::apply([&] (auto&...vals) {
            (serialize(vals, buffer), ...);
        }, value);
    }
    template <class T>
    void do_serialize(T const& value, span<std::byte>& buffer, value_tag<serialization_category::aggregate>) noexcept {
        serialize(as_tuple(value), buffer);
    }
}

template <class T>
void serialize(T const& value, span<std::byte>& buffer) noexcept {
    detail::do_serialize(value, buffer, serialization_category_tag_t<T>{});
}

// get_serialized_size

namespace detail {
    template <class T, serialization_category Category>
    constexpr void do_get_serialized_size(T const&, value_tag<Category>) noexcept {
        static_assert(serialization_category_v<T> != serialization_category::forbidden);
        static_assert(serialization_category_v<T> != serialization_category::unknown);
    }
    template <class T>
    constexpr size_t do_get_serialized_size(T const&, value_tag<serialization_category::trivial>) noexcept {
        return sizeof(T);
    }
    template <class T>
    constexpr size_t do_get_serialized_size(T const& array, value_tag<serialization_category::trivial_array>) noexcept {
        return sizeof(size_t) + array_traits<T>::size(array) * sizeof(typename array_traits<T>::value_type);
    }

    template <class T>
    constexpr size_t get_range_size(T const& range) noexcept {
        using traits = range_traits<T>;
        using value_type = typename traits::value_type;

        if constexpr (serialization_category_v<value_type> == serialization_category::trivial) {
            return traits::size(range) * sizeof(value_type);
        }
        else {
            size_t size = 0;
            std::for_each(traits::begin(range), traits::end(range), [&](auto& val) {
                size += get_serialized_size(val);
            });
            return size;
        }
    }

    template <class T>
    constexpr size_t do_get_serialized_size(T const& container, value_tag<serialization_category::container>) noexcept {
        return get_range_size(container) + sizeof(size_t);
    }
    template <class T>
    constexpr size_t do_get_serialized_size(T const& array, value_tag<serialization_category::fixed_array>) noexcept {
        return get_range_size(array);
    }
    template <class T>
    constexpr size_t do_get_serialized_size(T const& array, value_tag<serialization_category::dynamic_array>) noexcept {
        return get_range_size(array) + sizeof(size_t);
    }
    template <class T>
    constexpr size_t do_get_serialized_size(T const& value, value_tag<serialization_category::tuple>) noexcept {
        return std::apply([] (auto&...vals) {
            return (get_serialized_size(vals) + ... + 0);
        }, value);
    }
    template <class T>
    constexpr size_t do_get_serialized_size(T const& value, value_tag<serialization_category::aggregate>) noexcept {
        return get_serialized_size(as_tuple(value));
    }
}

template <class T>
constexpr size_t get_serialized_size(T const& value) noexcept {
    return detail::do_get_serialized_size(value, serialization_category_tag_t<T>{});
}

// deserialize

namespace detail {
    template <class T, serialization_category Category>
    void do_deserialize(T&, span<std::byte const>&, value_tag<Category>) noexcept {
        static_assert(serialization_category_v<T> != serialization_category::forbidden);
        static_assert(serialization_category_v<T> != serialization_category::unknown);
    }
    template <class T>
    void do_deserialize(T& value, span<std::byte const>& buffer, value_tag<serialization_category::trivial>) noexcept {
        memcpy(&value, buffer.data(), sizeof(T));
        buffer.begin() += sizeof(T);
    }
    template <class T>
    void do_deserialize(T& array, span<std::byte const>& buffer, value_tag<serialization_category::trivial_array>) {
        size_t count;
        deserialize(count, buffer);

        using traits = dynamic_array_traits<T>;
        traits::resize(array, count);

        const auto size = count * sizeof(typename traits::value_type);
        memcpy(traits::data(array), buffer.data(), size);
        buffer.begin() += size;
    }
    template <class T>
    void do_deserialize(T& container, span<std::byte const>& buffer, value_tag<serialization_category::container>) {
        size_t count;
        deserialize(count, buffer);

        for (size_t i = 0; i < count; ++i) {
            // TODO remove_deep_const_t version
            auto& value = container_traits<T>::emplace(container);
            deserialize(value, buffer);
        }
    }

    template <class T>
    void deserialize_array(T& array, size_t size, span<std::byte const>& buffer) {
        auto const data = array_traits<T>::data(array);
        for (auto ptr = data; ptr < data + size; ++ptr) {
            deserialize(*ptr, buffer);
        }
    }

    template <class T>
    void do_deserialize(T& array, span<std::byte const>& buffer, value_tag<serialization_category::fixed_array>) {
        deserialize_array(array, get_fixed_size<T>(), buffer);
    }
    template <class T>
    void do_deserialize(T& array, span<std::byte const>& buffer, value_tag<serialization_category::dynamic_array>) {
        size_t count;
        deserialize(count, buffer);

        dynamic_array_traits<T>::resize(array, count);
        deserialize_array(array, count, buffer);
    }
    template <class T>
    void do_deserialize(T& value, span<std::byte const>& buffer, value_tag<serialization_category::tuple>) {
        std::apply([&] (auto&...vals) {
            (deserialize(vals, buffer), ...);
        }, value);
    }
    template <class T>
    void do_deserialize(T& value, span<std::byte const>& buffer, value_tag<serialization_category::aggregate>) {
        auto tuple = as_tuple(value);
        deserialize(tuple, buffer);
    }
}

template <class T>
void deserialize(T& value, span<std::byte const>& buffer) {
    detail::do_deserialize(value, buffer, serialization_category_tag_t<T>{});
}

// try_get_deserialized_size

namespace detail {
    template <class T, serialization_category Category>
    void do_try_get_deserialized_size(span<std::byte const>, value_tag<Category>) noexcept {
        static_assert(serialization_category_v<T> != serialization_category::forbidden);
        static_assert(serialization_category_v<T> != serialization_category::unknown);
    }
    template <class T>
    std::pair<size_t, bool> do_try_get_deserialized_size(span<std::byte const> buffer, value_tag<serialization_category::trivial>) noexcept {
        return { sizeof(T), buffer.size() >= sizeof(T) };
    }

    template <class T>
    std::pair<size_t, bool> try_get_range_size(span<std::byte const> buffer) noexcept {
        auto const data_begin = buffer.begin();

        size_t count;
        if (buffer.size() < sizeof(count)) return { {}, false };
        deserialize(count, buffer);
        
        using value_type = typename range_traits<T>::value_type;
        if constexpr (serialization_category_v<value_type> == serialization_category::trivial) {
            auto const elements_size = count * sizeof(value_type);
            if (buffer.size() < elements_size) return { {}, false };
            buffer.begin() += elements_size;
        }
        else {
            for (size_t i = 0; i < count; ++i) {
                auto const [size, success] = try_get_deserialized_size<value_type>(buffer);
                if (!success) return { {}, false };
                buffer.begin() += size;
            }
        }
        return { buffer.begin() - data_begin, true };
    }

    template <class T>
    std::pair<size_t, bool> do_try_get_deserialized_size(span<std::byte const> buffer, value_tag<serialization_category::trivial_array>) noexcept {
        return try_get_range_size<T>(buffer);
    }
    template <class T>
    std::pair<size_t, bool> do_try_get_deserialized_size(span<std::byte const> buffer, value_tag<serialization_category::container>) noexcept {
        return try_get_range_size<T>(buffer);
    }
    template <class T>
    std::pair<size_t, bool> do_try_get_deserialized_size(span<std::byte const> buffer, value_tag<serialization_category::fixed_array>) noexcept {
        auto const data_begin = buffer.begin();

        using value_type = typename range_traits<T>::value_type;
        for (size_t i = 0; i < get_fixed_size<T>(); ++i) {
             auto const [size, success] = try_get_deserialized_size<value_type>(buffer);
            if (!success) return { {}, false };
            buffer.begin() += size;
        }
        return { buffer.begin() - data_begin, true };
    }
    template <class T>
    std::pair<size_t, bool> do_try_get_deserialized_size(span<std::byte const> buffer, value_tag<serialization_category::dynamic_array>) noexcept {
        return try_get_range_size<T>(buffer);
    }
    template <class T>
    std::pair<size_t, bool> do_try_get_deserialized_size(span<std::byte const> buffer, value_tag<serialization_category::tuple>) noexcept {
        return std::apply([&] (auto...tags) {
            auto const data_begin = buffer.begin();
            auto const try_one    = [&] (auto tag) {
                using value_type = typename decltype(tag)::type;
                auto const [size, success] = try_get_deserialized_size<value_type>(buffer);
                if (!success) return false;
                buffer.begin() += size;
                return true;
            };
            auto const success = (try_one(tags) && ...);
            return std::pair<size_t, bool>{ buffer.begin() - data_begin, success };
        }, map_tuple_types_t<T, tag_type>{});
    }
    template <class T>
    std::pair<size_t, bool> do_try_get_deserialized_size(span<std::byte const> buffer, value_tag<serialization_category::aggregate>) noexcept {
        using tuple_t = decltype(to_tuple(std::declval<T>()));
        return try_get_deserialized_size<tuple_t>(buffer);
    }
}

template <class T>
std::pair<size_t, bool> try_get_deserialized_size(span<std::byte const> buffer) noexcept {
    return detail::do_try_get_deserialized_size<T>(buffer, serialization_category_tag_t<T>{});
}
