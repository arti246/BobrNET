cmake_minimum_required(VERSION 3.15)
project(BobrNET)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к SQLite
set(SQLITE_DIR ${CMAKE_SOURCE_DIR}/external/sqlite)

# Пути к репозиториям
set(REPO_DIR ${CMAKE_SOURCE_DIR}/models/db/repositories)

# Сервер
add_executable(server
    server/server.cpp
    models/db/Database.cpp
    models/db/repositories/UserRepository.cpp
    models/db/repositories/ChatRepository.cpp
    models/db/repositories/MessageRepository.cpp
    models/db/repositories/LogRepository.cpp
    external/sqlite/sqlite3.c
)

# Клиент
add_executable(client
    client/client.cpp
)

# Где искать заголовочные файлы
target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${SQLITE_DIR}
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/models/db
    ${REPO_DIR}
)

target_include_directories(client PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/models
)

# Подключаем библиотеки
if(WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
else()
    target_link_libraries(server pthread)
endif()