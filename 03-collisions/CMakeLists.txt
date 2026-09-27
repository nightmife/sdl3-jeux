add_executable(collisions main.cpp)
target_compile_options(collisions PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(collisions PRIVATE SDL3::SDL3)
