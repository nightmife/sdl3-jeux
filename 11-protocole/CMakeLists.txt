# Programme en terminal, sans fenêtre : "protocole hote" ou "protocole client <adresse>"
add_executable(protocole main.cpp)
target_compile_options(protocole PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(protocole PRIVATE SDL3::SDL3 SDL3_net::SDL3_net)
