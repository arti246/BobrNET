#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <sstream>
#include <algorithm>
#include <chrono>
#include "../external/sqlite/sqlite3.h"

#include "../models/Platform.hpp"

#include "../models/User.hpp"
#include "../models/Message.hpp"
#include "../models/MessageStatus.hpp"
#include "../models/Chat.hpp"
#include "../server/Session.hpp"

#include "../models/db/Database.hpp"
#include "../models/db/repositories/ChatRepository.hpp"
#include "../models/db/repositories/UserRepository.hpp"
#include "../models/db/repositories/LogRepository.hpp"
#include "../models/db/repositories/MessageRepository.hpp"
#include "commands/CommandDispatcher.hpp"

// Глобальные объекты
Database g_db("server.db");
UserRepository g_users(g_db);
ChatRepository g_chats(g_db);
MessageRepository g_messages(g_db);
LogRepository g_logs(g_db);

// Активные сессии
std::map<int, std::shared_ptr<Session>> g_sessions;
std::mutex g_sessions_mutex;

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

void broadcast(const std::string& msg, int exclude_user_id = -1) {
    std::lock_guard<std::mutex> lock(g_sessions_mutex);
    for (auto& [uid, session] : g_sessions) {
        if (uid != exclude_user_id && session) {
            session->send(msg);
        }
    }
}

std::string hash_password(const std::string& password) {
    return password;  // TODO: добавить реальное хеширование
}

std::string get_chat_display_name(int chat_id, int current_user_id) {
    auto opt = g_chats.get_chat_by_id(chat_id);
    if (!opt.has_value()) return "Unknown";

    const Chat& chat = opt.value();

    if (chat.is_private()) {
        auto participants = g_chats.get_participants(chat_id, current_user_id);
        if (!participants.empty()) {
            return participants[0].login();
        }
        return "Private Chat";
    }

    return chat.name();
}

// ========== ОБРАБОТЧИК КЛИЕНТА ==========

void handle_client(std::shared_ptr<Session> session) {
    int user_id = session->user_id();
    std::string login = session->login();

    std::cout << "Client '" << login << "' (id=" << user_id << ") connected" << std::endl;
    g_logs.info("CONNECT", "User connected: " + login, user_id);

    // Отправляем недоставленные сообщения
    auto undelivered = g_messages.get_undelivered(user_id);
    for (const auto& msg : undelivered) {
        auto sender = g_users.find_by_id(msg.user_id());
        if (sender.has_value()) {
            session->send("[Delayed from " + sender->login() + "]: " + msg.text());
            g_messages.update_status(msg.id(), MessageStatus::DELIVERED);
        }
    }

    session->send("[Welcome back " + login + "! Type /help]");
    session->send_chat_list();

    CommandDispatcher dispatcher;

    char buffer[4096];
    while (true) {
        int bytes = recv(session->socket(), buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            break;
        }

        buffer[bytes] = '\0';
        std::string cmd_line(buffer);
        cmd_line.erase(std::remove(cmd_line.begin(), cmd_line.end(), '\n'), cmd_line.end());
        cmd_line.erase(std::remove(cmd_line.begin(), cmd_line.end(), '\r'), cmd_line.end());

        if (cmd_line.empty()) continue;

        std::cout << "Message from " << login << ": " << cmd_line << std::endl;

        if (!dispatcher.dispatch(*session, cmd_line)) {
            // Проверяем, есть ли активный чат
            if (session->active_chat() != -1) {
                session->send_message_to_active_chat(cmd_line);
            }
            else {
                session->send("Unknown command. Type /help. Or /open <chat_id> to enter a chat.");
            }
        }

        if (cmd_line == "/exit") break;
    }

    // Удаляем сессию
    {
        std::lock_guard<std::mutex> lock(g_sessions_mutex);
        g_sessions.erase(user_id);
    }

    broadcast(login + " left the chat", user_id);
    g_logs.info("DISCONNECT", "User disconnected: " + login, user_id);
    std::cout << "Client '" << login << "' disconnected" << std::endl;
    closesocket(session->socket());
}

// ========== MAIN ==========

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    // Инициализация БД
    if (!g_db.init()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return 1;
    }
    std::cout << "Database initialized" << std::endl;

    g_logs.info("STARTUP", "Server started on port 8888");

    // Создание сокета
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        g_logs.error("SOCKET_ERROR", "Socket creation failed", -1);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        g_logs.error("BIND_ERROR", "Bind failed on port 8888", -1);
        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        g_logs.error("LISTEN_ERROR", "Listen failed", -1);
        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    std::cout << "Server started on port 8888" << std::endl;
    std::cout << "Waiting for connections..." << std::endl;

    // Основной цикл
    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        // Получаем команду аутентификации (REGISTER или LOGIN)
        char buffer[512];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            closesocket(clientSocket);
            continue;
        }

        buffer[bytesReceived] = '\0';
        std::string cmd(buffer);
        cmd.erase(std::remove(cmd.begin(), cmd.end(), '\n'), cmd.end());
        cmd.erase(std::remove(cmd.begin(), cmd.end(), '\r'), cmd.end());

        std::istringstream iss(cmd);
        std::string action, login, password, birthday;
        iss >> action >> login >> password >> birthday;

        int user_id = -1;
        bool auth_success = false;

        if (action == "REGISTER") {
            if (g_users.create(login, hash_password(password), birthday)) {
                auto user = g_users.find_by_login(login);
                if (user.has_value()) {
                    user_id = user->id();
                    auth_success = true;
                    std::string msg = "[Registration successful! Welcome " + login + "]\n";
                    send(clientSocket, msg.c_str(), msg.size(), 0);
                    g_logs.info("REGISTER", "New user registered: " + login, user_id);
                }
            }
            else {
                std::string msg = "[Error: Username already taken]\n";
                send(clientSocket, msg.c_str(), msg.size(), 0);
            }
        }
        else if (action == "LOGIN") {
            auto user = g_users.find_by_login(login);
            if (user.has_value() && user->password_hash() == hash_password(password)) {
                user_id = user->id();
                auth_success = true;
                g_logs.info("LOGIN", "User logged in: " + login, user_id);
            }
            else {
                std::string msg = "[Error: Invalid login or password]\n";
                send(clientSocket, msg.c_str(), msg.size(), 0);
                g_logs.warning("LOGIN_FAILED", "Failed login attempt for " + login, -1);
            }
        }
        else {
            std::string msg = "[Error: First send REGISTER login pass or LOGIN login pass]\n";
            send(clientSocket, msg.c_str(), msg.size(), 0);
        }

        if (!auth_success) {
            closesocket(clientSocket);
            continue;
        }

        // Создаём сессию
        auto session = std::make_shared<Session>(clientSocket, user_id, login);

        // Сохраняем сессию
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            g_sessions[user_id] = session;
        }

        broadcast(login + " joined the chat", user_id);

        // Запускаем поток с shared_ptr
        std::thread client_thread(handle_client, session);
        client_thread.detach();
    }

    closesocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}