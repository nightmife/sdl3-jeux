# Le jeu en ligne : un hôte + jusqu'à 3 joueurs distants
add_executable(en_ligne main.cpp affichage.cpp session.cpp reseau.cpp messages.cpp jeu.cpp)
target_compile_options(en_ligne PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(en_ligne PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf SDL3_net::SDL3_net)

# Triches de l'hôte : ON pour ta version, OFF pour la version à donner à tes amis.
#   cmake -S . -B build-sans-triches -DAVEC_TRICHES=OFF
option(AVEC_TRICHES "Inclure les triches de l'hôte" ON)
if(AVEC_TRICHES)
    # Équivaut à écrire "#define AVEC_TRICHES" en haut de chaque fichier .cpp
    target_compile_definitions(en_ligne PRIVATE AVEC_TRICHES)
endif()

# Windows : intégrer les bibliothèques de base du C++ DANS le .exe, pour ne pas
# avoir à fournir libstdc++-6.dll et libgcc_s_seh-1.dll à côté
if(WIN32)
    target_link_options(en_ligne PRIVATE -static-libgcc -static-libstdc++)
endif()

# Copie le dossier assets/ à côté de l'exécutable après chaque compilation
add_custom_command(TARGET en_ligne POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:en_ligne>/assets)
