
#pragma once

#include <container_traits.hpp>
#include <cassert>

template <class T>
class span {
    T* begin_;
    T* end_;
public:
    template <class Array, class = std::enable_if_t<
        is_array_v<Array>
    >>
    span(Array&& arr) noexcept :
        begin_{ array_traits<remove_cvref_t<Array>>::data(arr) },
        end_  { array_traits<remove_cvref_t<Array>>::size(arr) + begin_ }
    {}

    span(T* data, size_t size) noexcept :
        begin_{ data },
        end_  { data + size }
    {}
    span(T* begin, T* end) noexcept :
        begin_{ begin },
        end_  { end }
    {}
    
    auto& data()       noexcept { return begin_; }
    auto  data() const noexcept { return begin_; }

    size_t size() const noexcept {
        auto const diff = end_ - begin_;
        assert(diff >= 0);
        return static_cast<size_t>(diff);
    }

    auto& begin()       noexcept { return begin_; }
    auto  begin() const noexcept { return begin_; }
    
    auto& end()       noexcept { return end_; }
    auto  end() const noexcept { return end_; }
};

template <class Array, class = std::enable_if_t<
    is_array_v<Array>
>>
span(Array&&) -> span<typename array_traits<remove_cvref_t<Array>>::value_type>;
