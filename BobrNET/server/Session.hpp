#pragma once

#include <string>
#include <vector>

#include "../models/Platform.hpp"

class Database;
class UserRepository;
class ChatRepository;
class MessageRepository;
class LogRepository;

class Session {
public:
    Session(SOCKET sock, int user_id, const std::string& login);
    ~Session();

    // Геттеры
    SOCKET socket() const { return m_socket; }
    int user_id() const { return m_user_id; }
    const std::string& login() const { return m_login; }

    // Отправка сообщения клиенту
    void send(const std::string& msg);

    // Команды (делегируют репозиториям)
    void send_chat_list();
    void send_chat_history(int chat_id);
    void send_message_to_user(const std::string& target_login, const std::string& text);
    void send_chat_history_with_user(const std::string& username);
    void disconnect();
    void set_active_chat(int chat_id);
    int active_chat() const;
    void send_message_to_active_chat(const std::string& text);

    // Доступ к репозиториям (через ссылки)
    Database& db();
    UserRepository& users();
    ChatRepository& chats();
    MessageRepository& messages();
    LogRepository& logs();

private:
    SOCKET m_socket;
    int m_user_id;
    std::string m_login;
};