cmake_minimum_required(VERSION 3.16)
project(BobrNET)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Сначала глобальные настройки Qt (до создания таргетов)
set(CMAKE_PREFIX_PATH "D:/PM/QT/6.11.1/msvc2022_64")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 2. Находим пакеты
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

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
    ${CMAKE_SOURCE_DIR}/external/sqlite
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_SOURCE_DIR}/models/db
    ${CMAKE_SOURCE_DIR}/models/db/repositories
)

# ========== КЛИЕНТ (консольный) ==========
add_executable(client
    client/client_console.cpp
    client/gui/QtNetworkAdapter.cpp
    client/NetworkClient.cpp)

target_include_directories(client PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/models
)

# Для работы QObject в консольном клиенте
target_link_libraries(client PRIVATE Qt6::Core ws2_32)

# ========== QT GUI КЛИЕНТ ==========
add_executable(client_gui
    client/gui/main.cpp
    client/gui/MainWindow.cpp
    client/gui/LoginDialog.cpp
    client/gui/QtNetworkAdapter.cpp
    client/NetworkClient.cpp
    client/gui/QtNetworkAdapter.h
)

target_include_directories(client_gui PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/client
    ${CMAKE_SOURCE_DIR}/client/gui
    ${CMAKE_SOURCE_DIR}/models
)

# Линкуем Qt библиотеки (инклюды подключатся автоматически через линковку)
target_link_libraries(client_gui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ws2_32
)

# ========== СИСТЕМНЫЕ ЗАВИСИМОСТИ ==========
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
else()
    target_link_libraries(server PRIVATE pthread)
endif()

# ========== РАЗВЕРТЫВАНИЕ (DEPLOY) ==========
# Вместо ручного копирования DLL лучше использовать встроенный механизм
if(WIN32)
    add_custom_command(TARGET client_gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_PREFIX_PATH}/plugins/platforms"
            $<TARGET_FILE_DIR:client_gui>/platforms
    )
endif()
