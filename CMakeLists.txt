cmake_minimum_required(VERSION 3.20)
project(sdl3_jeux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)

# Une leçon = un sous-dossier = un exécutable
add_subdirectory(01-fenetre)
add_subdirectory(01b-classique)
add_subdirectory(02-mouvement)
add_subdirectory(03-collisions)
add_subdirectory(04-separation)
add_subdirectory(05-textures)
add_subdirectory(06-texte)
add_subdirectory(07-son)
add_subdirectory(08-etats)
