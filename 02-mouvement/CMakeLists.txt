add_executable(mouvement main.cpp)
target_compile_options(mouvement PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(mouvement PRIVATE SDL3::SDL3)
