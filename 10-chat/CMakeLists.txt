# Programme en terminal, sans fenêtre : "chat hote" ou "chat client <adresse>"
add_executable(chat main.cpp)
target_compile_options(chat PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(chat PRIVATE SDL3::SDL3 SDL3_net::SDL3_net)
