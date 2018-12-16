
#pragma once

#include <serialization.hpp>

namespace detail {
    template <class StreamDerived, class SpanBase, class ErrorPolicy>
    struct stream_mixin_base : ErrorPolicy {
        auto&       base()       noexcept { return static_cast<StreamDerived&>      (*this); }
        auto const& base() const noexcept { return static_cast<StreamDerived const&>(*this); }

        auto&       span()       noexcept { return static_cast<SpanBase&>      (base()); }
        auto const& span() const noexcept { return static_cast<SpanBase const&>(base()); }
    };

    template <class StreamDerived, class SpanBase, class ErrorPolicy>
    struct istream_mixin : stream_mixin_base<StreamDerived, SpanBase, ErrorPolicy> {};
    template <class StreamDerived, class SpanBase, class ErrorPolicy>
    struct ostream_mixin : stream_mixin_base<StreamDerived, SpanBase, ErrorPolicy> {};
}

template <class ErrorPolicy>
struct basic_binary_istream :
    span<std::byte const>,
    detail::istream_mixin<basic_binary_istream<ErrorPolicy>, span<const std::byte>, ErrorPolicy>
{
    using span::span;
};
template <class ErrorPolicy>
struct basic_binary_ostream :
    span<std::byte>,
    detail::ostream_mixin<basic_binary_ostream<ErrorPolicy>, span<std::byte>, ErrorPolicy>
{
    using span::span;
};
template <class ErrorPolicy>
struct basic_binary_stream :
    span<std::byte>,
    detail::istream_mixin<basic_binary_stream<ErrorPolicy>, span<std::byte>, ErrorPolicy>,
    detail::ostream_mixin<basic_binary_stream<ErrorPolicy>, span<std::byte>, ErrorPolicy>
{
    using span::span;
};

struct unchecked_serialization_policy {};
struct throwing_serialization_policy  {};
struct fail_flag_serialization_policy { bool overflow = false; };

using unchecked_binary_istream = basic_binary_istream<unchecked_serialization_policy>;
using unchecked_binary_ostream = basic_binary_ostream<unchecked_serialization_policy>;
using unchecked_binary_stream  = basic_binary_stream<unchecked_serialization_policy>;

using throwing_binary_istream = basic_binary_istream<throwing_serialization_policy>;
using throwing_binary_ostream = basic_binary_ostream<throwing_serialization_policy>;
using throwing_binary_stream  = basic_binary_stream<throwing_serialization_policy>;

using binary_istream = basic_binary_istream<fail_flag_serialization_policy>;
using binary_ostream = basic_binary_ostream<fail_flag_serialization_policy>;
using binary_stream  = basic_binary_stream<fail_flag_serialization_policy>;

namespace detail {
    template <class T, class StreamDerived, class SpanBase>
    StreamDerived& operator<<(ostream_mixin<StreamDerived, SpanBase, unchecked_serialization_policy>& stream, T const& value) {
        serialize(value, stream.span());
        return stream.base();
    }
    template <class T, class StreamDerived, class SpanBase>
    StreamDerived& operator<<(ostream_mixin<StreamDerived, SpanBase, throwing_serialization_policy>& stream, T const& value) {
        auto const size = get_serialized_size(value);
        if (size > stream.span().size()) throw std::runtime_error{"Tried to overflow binary ostream"};
        serialize(value, stream.span());
        return stream.base();
    }
    template <class T, class StreamDerived, class SpanBase>
    StreamDerived& operator<<(ostream_mixin<StreamDerived, SpanBase, fail_flag_serialization_policy>& stream, T const& value) {
        if (stream.overflow) return stream.base();

        auto const size = get_serialized_size(value);
        if (size > stream.span().size()) {
            stream.overflow = true;
            return stream.base();
        }
        serialize(value, stream.span());
        return stream.base();
    }
    
    template <class T, class StreamDerived, class SpanBase>
    StreamDerived& operator>>(istream_mixin<StreamDerived, SpanBase, unchecked_serialization_policy>& stream, T& value) {
        deserialize(value, stream.span());
        return stream.base();
    }
    template <class T, class StreamDerived, class SpanBase>
    StreamDerived& operator>>(istream_mixin<StreamDerived, SpanBase, throwing_serialization_policy>& stream, T& value) {
        auto const [_, success] = try_get_deserialized_size<T>(stream.span());
        if (!success) throw std::runtime_error{"Tried to overflow binary istream"};
        deserialize(value, stream.span());
        return stream.base();
    }
    template <class T, class StreamDerived, class SpanBase>
    StreamDerived& operator>>(istream_mixin<StreamDerived, SpanBase, fail_flag_serialization_policy>& stream, T& value) {
        if (stream.overflow) return stream.base();

        auto const [_, success] = try_get_deserialized_size<T>(stream.span());
        if (!success) {
            stream.overflow = true;
            return stream.base();
        }
        deserialize(value, stream.span());
        return stream.base();
    }
}
