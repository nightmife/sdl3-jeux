# Le jeu en ligne : un hôte + jusqu'à 3 joueurs distants
add_executable(en_ligne main.cpp affichage.cpp session.cpp reseau.cpp messages.cpp jeu.cpp)
target_compile_options(en_ligne PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(en_ligne PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf SDL3_net::SDL3_net)

# Copie le dossier assets/ à côté de l'exécutable après chaque compilation
add_custom_command(TARGET en_ligne POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:en_ligne>/assets)
