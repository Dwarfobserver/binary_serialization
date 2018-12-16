
#include <binary_stream.hpp>
#include <vector>

int main() {
    auto vec = std::vector{ 1, 2, 3 };
    auto buf  = std::array<std::byte, 50>{};

	static_assert(serialization_category_v<decltype(vec)> == serialization_category::trivial_array);

    auto ostream = binary_ostream{buf};
    ostream << vec;

    auto const vec_size = get_serialized_size(vec);
    assert(vec_size == sizeof(size_t) + sizeof(int) * vec.size());
    assert(vec_size == static_cast<size_t>(ostream.data() - buf.data()));

    auto vec_copy = decltype(vec){};
    auto istream = binary_istream{buf};
    istream >> vec_copy;

    assert(vec == vec_copy);
    assert(vec_size == static_cast<size_t>(istream.data() - buf.data()));
}


