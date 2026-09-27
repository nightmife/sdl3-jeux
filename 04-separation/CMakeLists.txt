# jeu.cpp = logique pure (sans SDL), main.cpp = fenêtre, clavier, dessin
add_executable(separation main.cpp jeu.cpp)
target_compile_options(separation PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(separation PRIVATE SDL3::SDL3)
