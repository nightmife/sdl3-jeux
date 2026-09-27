add_executable(fenetre main.cpp)
target_compile_options(fenetre PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(fenetre PRIVATE SDL3::SDL3)
