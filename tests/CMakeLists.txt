
add_executable       (tests main.cpp)
target_link_libraries(tests binary_serialization)
add_test        (NAME tests COMMAND tests)
