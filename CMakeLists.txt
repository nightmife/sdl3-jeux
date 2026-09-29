cmake_minimum_required(VERSION 3.20)
project(sdl3_jeux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)

# SDL3_net n'est pas packagée sur Arch : CMake télécharge la version 3.2.0
# et la compile avec le projet (en bibliothèque statique, sans les exemples).
include(FetchContent)
set(SDLNET_SAMPLES OFF)
set(BUILD_SHARED_LIBS OFF)
FetchContent_Declare(SDL3_net
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_net.git
    GIT_TAG        release-3.2.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(SDL3_net)

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
add_subdirectory(09-pas-fixe)
add_subdirectory(10-chat)
add_subdirectory(11-protocole)
add_subdirectory(12-messages)
add_subdirectory(13-multi)
add_subdirectory(14-etat)
add_subdirectory(15-en-ligne)
