
#include <binary_stream.hpp>
#include <vector>

int main() {
    auto const vec = std::vector{ 1, 2, 3 };
    static_assert(serialization_category_v<std::vector<int>> == serialization_category::trivial_array);

    auto buffer  = std::array<std::byte, 50>{};
    auto ostream = binary_ostream{ buffer };
    ostream << vec;

    auto const vec_size = get_serialized_size(vec);
    assert(vec_size == sizeof(size_t) + sizeof(int) * vec.size());
    assert(vec_size == static_cast<size_t>(ostream.data() - buffer.data()));

    auto vec_copy = decltype(vec){};
    auto istream = binary_istream{ buffer };
    istream >> vec_copy;

    assert(vec == vec_copy);
    assert(ostream.data() == istream.data());
}


