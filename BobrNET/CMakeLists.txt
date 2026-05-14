cmake_minimum_required(VERSION 3.15)
project(BobrNET)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Сервер (пути указываем явно)
add_executable(server
    server/server.cpp
    models/db/database.cpp
    external/sqlite/sqlite3.c
 "models/db/repositories/ChatRepository.cpp" "models/db/repositories/MessageRepository.cpp" "models/db/repositories/UserRepository.cpp")

# Клиент
add_executable(client
    client/client.cpp
 "models/db/repositories/ChatRepository.cpp" "models/db/repositories/MessageRepository.cpp" "models/db/repositories/UserRepository.cpp")

# Пути к заголовкам
target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/sqlite
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/models/db
)

target_include_directories(client PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/models
)

# Библиотеки
if(WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
endif()