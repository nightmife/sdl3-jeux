add_executable(classique main.cpp)
target_compile_options(classique PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(classique PRIVATE SDL3::SDL3)
