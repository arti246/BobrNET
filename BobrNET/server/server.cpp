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

#include "../models/db/Database.hpp"
#include "../models/db/repositories/ChatRepository.hpp"
#include "../models/db/repositories/UserRepository.hpp"
#include "../models/db/repositories/LogRepository.hpp"
#include "../models/db/repositories/MessageRepository.hpp"

// Глобальные объекты
Database db("server.db");
UserRepository users(db);
ChatRepository chats(db);
MessageRepository messages(db);
LogRepository logs(db);

// Активные сессии (user_id -> socket)
std::map<int, SOCKET> active_sessions;
std::mutex sessions_mutex;

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

void send_to_client(SOCKET sock, const std::string& msg) {
    send(sock, msg.c_str(), msg.size(), 0);
}

void broadcast(const std::string& msg, SOCKET exclude = INVALID_SOCKET) {
    std::lock_guard<std::mutex> lock(sessions_mutex);
    for (auto& pair : active_sessions) {
        if (pair.second != exclude) {
            send_to_client(pair.second, msg);
        }
    }
}

std::string hash_password(const std::string& password) {
    return password;  // TODO: добавить реальное хеширование
}

std::string get_chat_display_name(int chat_id, int current_user_id) {
    auto opt = chats.get_chat_by_id(chat_id);
    if (!opt.has_value()) return "Unknown";

    const Chat& chat = opt.value();

    if (chat.is_private()) {
        auto participants = chats.get_participants(chat_id, current_user_id);
        if (!participants.empty()) {
            return participants[0].login();
        }
        return "Private Chat";
    }

    return chat.name();
}

void send_chat_list(SOCKET clientSocket, int user_id) {
    auto user_chats = chats.get_user_chats(user_id);
    if (user_chats.empty()) {
        send_to_client(clientSocket, "[No chats yet. Send a message to someone to create a chat]");
        return;
    }

    send_to_client(clientSocket, "=== Your chats ===");
    for (const auto& chat : user_chats) {
        int unread = messages.get_unread_count(chat.id(), user_id);
        std::string unread_mark = (unread > 0) ? " (" + std::to_string(unread) + " new)" : "";
        std::string display_name = get_chat_display_name(chat.id(), user_id);
        send_to_client(clientSocket, "  [" + std::to_string(chat.id()) + "] " + display_name + unread_mark);
    }
    send_to_client(clientSocket, "=================");
}

void send_chat_history(SOCKET clientSocket, int chat_id, int user_id) {
    auto opt = chats.get_chat_by_id(chat_id);
    if (!opt.has_value()) {
        send_to_client(clientSocket, "[Error: Chat not found]");
        return;
    }

    const Chat& chat = opt.value();

    if (!chats.is_participant(chat_id, user_id)) {
        send_to_client(clientSocket, "[Error: You are not a member of this chat]");
        return;
    }

    auto chat_messages = messages.get_by_chat(chat_id, 50);
    if (chat_messages.empty()) {
        send_to_client(clientSocket, "[No messages in this chat yet]");
        return;
    }

    std::string title = "=== Chat: " + get_chat_display_name(chat_id, user_id) + " ===";
    send_to_client(clientSocket, title);

    // Выводим сообщения в хронологическом порядке
    std::vector<Message> reversed;
    for (auto it = chat_messages.rbegin(); it != chat_messages.rend(); ++it) {
        reversed.push_back(*it);
    }

    for (const auto& msg : reversed) {
        auto sender = users.find_by_id(msg.user_id());
        std::string sender_name = sender.has_value() ? sender->login() : "unknown";
        send_to_client(clientSocket, sender_name + ": " + msg.text());
    }

    send_to_client(clientSocket, "=========================");

    // Отмечаем сообщения как прочитанные
    messages.mark_all_in_chat_as_read(chat_id, user_id);
}

// ========== КОМАНДЫ (ОТДЕЛЬНЫЕ ФУНКЦИИ) ==========

void cmd_msg(SOCKET clientSocket, const std::string& args, int user_id, const std::string& login) {
    // Формат: username text
    size_t space = args.find(' ');
    if (space == std::string::npos) {
        send_to_client(clientSocket, "[Error: /msg username text]");
        return;
    }

    std::string target_login = args.substr(0, space);
    std::string text = args.substr(space + 1);

    auto target_user = users.find_by_login(target_login);
    if (!target_user.has_value()) {
        send_to_client(clientSocket, "[Error: User '" + target_login + "' does not exist]");
        return;
    }

    // Находим или создаём личный чат
    int chat_id = chats.find_private_chat(user_id, target_user->id());
    if (chat_id == -1) {
        chats.create_private_chat(user_id, target_user->id(), chat_id);
        logs.info("CHAT_CREATED", "Private chat created between " + login + " and " + target_login, user_id);
    }

    // Сохраняем сообщение
    if (!messages.save(chat_id, user_id, text, MessageStatus::SENT)) {
        send_to_client(clientSocket, "[Error: Failed to save message]");
        return;
    }

    // Отправляем получателю, если он онлайн
    bool delivered = false;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        auto it = active_sessions.find(target_user->id());
        if (it != active_sessions.end()) {
            send_to_client(it->second, login + ": " + text);
            delivered = true;
            messages.mark_all_in_chat_as_delivered(chat_id, target_user->id());
        }
    }

    if (delivered) {
        send_to_client(clientSocket, "[Sent to " + target_login + "]");
    }
    else {
        send_to_client(clientSocket, "[Saved for offline delivery to " + target_login + "]");
    }
    logs.info("MESSAGE_SENT", login + " -> " + target_login + ": " + text, user_id);
}

void cmd_chats(SOCKET clientSocket, const std::string&, int user_id, const std::string&) {
    send_chat_list(clientSocket, user_id);
}

void cmd_open(SOCKET clientSocket, const std::string& args, int user_id, const std::string&) {
    if (args.empty()) {
        send_to_client(clientSocket, "[Error: /open chat_id]");
        return;
    }
    try {
        int chat_id = std::stoi(args);
        send_chat_history(clientSocket, chat_id, user_id);
    }
    catch (...) {
        send_to_client(clientSocket, "[Error: Invalid chat ID]");
    }
}

void cmd_history(SOCKET clientSocket, const std::string& args, int user_id, const std::string&) {
    if (args.empty()) {
        send_to_client(clientSocket, "[Error: /history username]");
        return;
    }

    auto target_user = users.find_by_login(args);
    if (!target_user.has_value()) {
        send_to_client(clientSocket, "[Error: User '" + args + "' does not exist]");
        return;
    }

    int chat_id = chats.find_private_chat(user_id, target_user->id());
    if (chat_id == -1) {
        send_to_client(clientSocket, "[No chat history with " + args + "]");
    }
    else {
        send_chat_history(clientSocket, chat_id, user_id);
    }
}

void cmd_help(SOCKET clientSocket, const std::string&, int, const std::string&) {
    send_to_client(clientSocket, "=== Commands ===");
    send_to_client(clientSocket, "  /msg <user> <text> - send message");
    send_to_client(clientSocket, "  /chats - list your chats");
    send_to_client(clientSocket, "  /open <chat_id> - open chat and show history");
    send_to_client(clientSocket, "  /history <user> - show chat history with user");
    send_to_client(clientSocket, "  /exit - disconnect");
    send_to_client(clientSocket, "=================");
}

void cmd_exit(SOCKET clientSocket, const std::string&, int, const std::string&) {
    send_to_client(clientSocket, "[Goodbye!]");
}

// ========== ОБРАБОТЧИК КЛИЕНТА ==========

void handle_client(SOCKET clientSocket, int user_id, const std::string& login) {
    std::cout << "Client '" << login << "' (id=" << user_id << ") connected" << std::endl;
    logs.info("CONNECT", "User connected: " + login, user_id);

    // Отправляем недоставленные сообщения
    auto undelivered = messages.get_undelivered(user_id);
    for (const auto& msg : undelivered) {
        auto sender = users.find_by_id(msg.user_id());
        if (sender.has_value()) {
            send_to_client(clientSocket, "[Delayed from " + sender->login() + "]: " + msg.text());
            messages.update_status(msg.id(), MessageStatus::DELIVERED);
        }
    }

    // Приветствие и список чатов
    send_to_client(clientSocket, "[Welcome " + login + "! Type /help for commands]");
    send_chat_list(clientSocket, user_id);

    char buffer[4096];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string msg(buffer);

        // Убираем перевод строки
        msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());
        msg.erase(std::remove(msg.begin(), msg.end(), '\r'), msg.end());

        if (msg.empty()) continue;

        std::cout << "Message from " << login << ": " << msg << std::endl;

        // Парсим команду
        size_t space = msg.find(' ');
        std::string cmd = (space != std::string::npos) ? msg.substr(0, space) : msg;
        std::string args = (space != std::string::npos) ? msg.substr(space + 1) : "";

        // Таблица команд (если добавить новую команду — просто добавь сюда условие)
        bool handled = true;

        if (cmd == "/msg") {
            cmd_msg(clientSocket, args, user_id, login);
        }
        else if (cmd == "/chats" || cmd == "/list") {
            cmd_chats(clientSocket, args, user_id, login);
        }
        else if (cmd == "/open") {
            cmd_open(clientSocket, args, user_id, login);
        }
        else if (cmd == "/history") {
            cmd_history(clientSocket, args, user_id, login);
        }
        else if (cmd == "/help") {
            cmd_help(clientSocket, args, user_id, login);
        }
        else if (cmd == "/exit") {
            cmd_exit(clientSocket, args, user_id, login);
            break;
        }
        else {
            send_to_client(clientSocket, "[Unknown command: " + cmd + ". Type /help]");
        }
    }

    // Удаляем сессию
    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        active_sessions.erase(user_id);
    }

    logs.info("DISCONNECT", "User disconnected: " + login, user_id);
    std::cout << "Client '" << login << "' disconnected" << std::endl;
    closesocket(clientSocket);
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    // Инициализация БД
    if (!db.init()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return 1;
    }
    std::cout << "Database initialized" << std::endl;

    logs.info("STARTUP", "Server started on port 8888");

    // Создание сокета
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        logs.error("SOCKET_ERROR", "Socket creation failed", -1);
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
        logs.error("BIND_ERROR", "Bind failed on port 8888", -1);
        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        logs.error("LISTEN_ERROR", "Listen failed", -1);
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
        std::string action, login, password;
        iss >> action >> login >> password;

        int user_id = -1;
        bool auth_success = false;

        if (action == "REGISTER") {
            if (users.create(login, hash_password(password))) {
                auto user = users.find_by_login(login);
                if (user.has_value()) {
                    user_id = user->id();
                    auth_success = true;
                    send_to_client(clientSocket, "[Registration successful! Welcome " + login + "]");
                    logs.info("REGISTER", "New user registered: " + login, user_id);
                }
            }
            else {
                send_to_client(clientSocket, "[Error: Username already taken]");
            }
        }
        else if (action == "LOGIN") {
            auto user = users.find_by_login(login);
            if (user.has_value() && user->password_hash() == hash_password(password)) {
                user_id = user->id();
                auth_success = true;
                send_to_client(clientSocket, "[Login successful! Welcome back " + login + "]");
                logs.info("LOGIN", "User logged in: " + login, user_id);
            }
            else {
                send_to_client(clientSocket, "[Error: Invalid login or password]");
                logs.warning("LOGIN_FAILED", "Failed login attempt for " + login, -1);
            }
        }
        else {
            send_to_client(clientSocket, "[Error: First send REGISTER login pass or LOGIN login pass]");
        }

        if (!auth_success) {
            closesocket(clientSocket);
            continue;
        }

        // Сохраняем сессию
        {
            std::lock_guard<std::mutex> lock(sessions_mutex);
            active_sessions[user_id] = clientSocket;
        }

        // Запускаем поток обработки клиента
        std::thread client_thread(handle_client, clientSocket, user_id, login);
        client_thread.detach();
    }

    closesocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}