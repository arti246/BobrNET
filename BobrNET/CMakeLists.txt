cmake_minimum_required(VERSION 3.15)
project(Messenger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Сервер
add_executable(server 
    server/server.cpp 
    external/sqlite/sqlite3.c
)

# Клиент
add_executable(client client/client.cpp)

# Включаем заголовки SQLite
target_include_directories(server PRIVATE external/sqlite)

# Для Windows подключаем библиотеку сокетов
if(WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
else()
    target_link_libraries(server pthread)
endif()