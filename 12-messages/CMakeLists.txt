# reseau.* = ton protocole (taille + données), messages.* = encodage des données du jeu
add_executable(messages main.cpp reseau.cpp messages.cpp)
target_compile_options(messages PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(messages PRIVATE SDL3::SDL3 SDL3_net::SDL3_net)
