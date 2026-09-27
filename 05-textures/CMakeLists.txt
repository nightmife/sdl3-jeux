add_executable(textures main.cpp jeu.cpp)
target_compile_options(textures PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(textures PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

# Copie le dossier assets/ à côté de l'exécutable après chaque compilation,
# pour que le jeu trouve ses images quel que soit le dossier d'où on le lance.
add_custom_command(TARGET textures POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:textures>/assets)
