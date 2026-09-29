add_executable(multi main.cpp jeu.cpp)
target_compile_options(multi PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(multi PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf)

# Copie le dossier assets/ à côté de l'exécutable après chaque compilation,
# pour que le jeu trouve ses images quel que soit le dossier d'où on le lance.
add_custom_command(TARGET multi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:multi>/assets)
