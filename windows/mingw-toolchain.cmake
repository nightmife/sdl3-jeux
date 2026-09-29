# Fichier de "chaîne de compilation" (toolchain) : dit à CMake de compiler
# POUR Windows (64 bits) DEPUIS Linux, avec le compilateur croisé MinGW.
# Utilisation : cmake -S . -B build-windows -DCMAKE_TOOLCHAIN_FILE=windows/mingw-toolchain.cmake

# 1. La cible : Windows, processeur x86_64 (le système SUR LEQUEL le jeu tournera)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# 2. Les compilateurs croisés (paquet Arch : mingw-w64-gcc)
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# 3. Où chercher les bibliothèques POUR WINDOWS : le système MinGW, et les
#    versions Windows de SDL téléchargées dans windows/deps
set(DEPS ${CMAKE_CURRENT_LIST_DIR}/deps)
set(CMAKE_FIND_ROOT_PATH
    /usr/x86_64-w64-mingw32
    ${DEPS}/SDL3-3.4.16/x86_64-w64-mingw32
    ${DEPS}/SDL3_image-3.4.6/x86_64-w64-mingw32
    ${DEPS}/SDL3_ttf-3.2.2/x86_64-w64-mingw32)

# 4. Ne JAMAIS utiliser les bibliothèques Linux de ton système (/usr/lib) :
#    elles ne marcheraient pas sous Windows. Les programmes (outils de build),
#    eux, restent ceux de Linux, puisque c'est Linux qui compile.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
