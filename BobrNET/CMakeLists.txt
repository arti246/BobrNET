cmake_minimum_required(VERSION 3.16)
project(BobrNET)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к SQLite
set(SQLITE_DIR ${CMAKE_SOURCE_DIR}/external/sqlite)

# Пути к репозиториям
set(REPO_DIR ${CMAKE_SOURCE_DIR}/models/db/repositories)

# ========== СЕРВЕР ==========
add_executable(server
    server/server.cpp
    server/Session.cpp
    server/commands/MsgCommand.cpp
    server/commands/ChatsCommand.cpp
    server/commands/CommandDispatcher.cpp
    server/commands/OpenCommand.cpp
    server/commands/HistoryCommand.cpp
    server/commands/HelpCommand.cpp
    server/commands/ExitCommand.cpp
    models/db/Database.cpp
    models/db/repositories/UserRepository.cpp
    models/db/repositories/ChatRepository.cpp
    models/db/repositories/MessageRepository.cpp
    models/db/repositories/LogRepository.cpp
    external/sqlite/sqlite3.c
)

target_include_directories(server PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${SQLITE_DIR}
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/models/db
    ${REPO_DIR}
)

# ========== КЛИЕНТ (консольный) ==========
add_executable(client
    client/client_console.cpp
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

# ========== QT GUI КЛИЕНТ ==========
if(TRUE)
    set(CMAKE_PREFIX_PATH "D:/PM/QT/6.11.1/msvc2022_64")
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

    add_executable(client_gui
        client/gui/main.cpp
        client/gui/MainWindow.cpp
        client/gui/LoginDialog.cpp
        client/NetworkClient.cpp
    )

    target_include_directories(client_gui PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/client
        ${CMAKE_SOURCE_DIR}/client/gui
        ${CMAKE_SOURCE_DIR}/client/network
        ${CMAKE_SOURCE_DIR}/models
    )

    target_link_libraries(client_gui
        Qt6::Core
        Qt6::Widgets
        ws2_32
    )

    set_target_properties(client_gui PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
    )
endif()