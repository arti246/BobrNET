cmake_minimum_required(VERSION 3.15)
project(Messenger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Сервер
add_executable(server server/server.cpp)

# Клиент
add_executable(client client/client.cpp)

# Для Windows подключаем библиотеку сокетов
if(WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
endif()