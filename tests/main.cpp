
#include <binary_stream.hpp>
#include <vector>
#include <string>
#include <list>

auto buffer = std::array<std::byte, 1000>{};

template <class T>
void test(T const& value, serialization_category category, size_t serial_size) {
    assert(serialization_category_v<T> == category);

    auto ostream = binary_ostream{ buffer };
    ostream << value;
    assert(!ostream.overflow);

    size_t const size = ostream.data() - buffer.data();
    assert(size == get_serialized_size(value));
    assert(size == serial_size);

    auto value_copy = T{};
    auto istream = binary_istream{ buffer };
    istream >> value_copy;

    assert(ostream.data() == istream.data());
    assert(value == value_copy);

    ostream = { buffer.data(), serial_size - 1 };
    ostream << value;
    assert(ostream.overflow);
}

struct vec2i {
    int x, y;

    bool operator==(vec2i const& rhs) const noexcept {
        return as_tuple(*this) == as_tuple(rhs);
    }
};

struct person {
    std::string name;
    int age;

    bool operator==(person const& rhs) const noexcept {
        return as_tuple(*this) == as_tuple(rhs);
    }
};

int main() {
    test(vec2i{ 3, 4 },          serialization_category::trivial,       sizeof(vec2i));
    //test(std::list{ 1, 2, 3 },   serialization_category::container,     sizeof(size_t) + 3 * sizeof(int));
    test(std::vector{ 1, 2, 3 }, serialization_category::trivial_array, sizeof(size_t) + 3 * sizeof(int));
    //test(person{ "Lily", 24 },   serialization_category::aggregate,     sizeof(size_t) + 4 + sizeof(int));
}


