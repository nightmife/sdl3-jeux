cmake_minimum_required(VERSION 3.20)
project(sdl3_jeux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)

# Une leçon = un sous-dossier = un exécutable
add_subdirectory(01-fenetre)
