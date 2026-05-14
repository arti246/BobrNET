#pragma once

#include "../../external/sqlite/sqlite3.h"
#include <string>
#include <vector>
#include <optional>

#include "../User.hpp"
#include "../Message.hpp"
#include "../Chat.hpp"

class Database {
public:
    Database(const std::string& path);
    ~Database();

    // Инициализация таблиц
    bool init();

    // Пользователи
    bool create_user(const std::string& login, const std::string& password_hash);
    std::optional<User> find_user_by_login(const std::string& login);
    std::optional<User> find_user_by_id(int id);

    // Сообщения
    bool save_message(int from_id, int to_id, const std::string& text, MessageStatus status = MessageStatus::SENT);
    bool update_message_status(int message_id, MessageStatus status);
    std::vector<Message> get_messages_between(int user1_id, int user2_id, int limit = 100);
    std::vector<Message> get_undelivered_messages(int user_id);  // для оффлайн-доставки

    // Логи
    bool log_event(const std::string& level, const std::string& event_type, const std::string& message, int user_id = -1);

    // Чаты
    bool create_private_chat(int user1_id, int user2_id, int& chat_id);
    bool create_group_chat(const std::string& name, const std::vector<int>& user_ids, int& chat_id);
    std::vector<Chat> get_user_chats(int user_id);
    std::vector<User> get_chat_participants(int chat_id, int exclude_user_id = -1);
    int get_private_chat_between(int user1_id, int user2_id);  // вернёт chat_id или -1

    // Сообщения с поддержкой чатов
    bool save_message(int chat_id, int from_id, const std::string& text);
    std::vector<Message> get_messages_in_chat(int chat_id, int limit = 50);
    bool mark_messages_as_read(int chat_id, int user_id);  // все сообщения в чате, где user_id НЕ отправитель

private:
    sqlite3* m_db;

    bool execute_sql(const std::string& sql);
    bool prepare_and_bind(sqlite3_stmt* stmt, ...);
};