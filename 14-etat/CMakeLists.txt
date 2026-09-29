# Test de la sérialisation de l'état du jeu, SANS réseau ni SDL
add_executable(etat main.cpp messages.cpp jeu.cpp)
target_compile_options(etat PRIVATE -Wall -Wextra -Wpedantic)
