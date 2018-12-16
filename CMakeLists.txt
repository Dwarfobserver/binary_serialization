
cmake_minimum_required(VERSION 3.10)
project(binary_serialization)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
enable_testing()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_subdirectory(src)
add_subdirectory(tests)
