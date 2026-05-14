#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../../../external/sqlite/sqlite3.h"

class Database;
class Message;

class MessageRepository {
public:
    explicit MessageRepository(Database& db);

    // Сохранение сообщения
    bool save(int chat_id, int from_user_id, const std::string& text, int status = 0);
    bool save(const Message& message);

    // Получение сообщений из чата
    std::vector<Message> get_by_chat(int chat_id, int limit = 50, int offset = 0);
    std::vector<Message> get_after_timestamp(int chat_id, long long timestamp, int limit = 50);

    // Оффлайн-сообщения
    std::vector<Message> get_undelivered(int user_id);

    // Обновление статуса
    bool update_status(int message_id, int status);
    bool mark_as_delivered(int message_id);
    bool mark_as_read(int message_id);

    // Для личных чатов (для обратной совместимости)
    std::vector<Message> get_between_users(int user1_id, int user2_id, int limit = 50);

    // Обновить все сообщения в чате как доставленные/прочитанные
    bool mark_all_in_chat_as_delivered(int chat_id, int except_user_id);
    bool mark_all_in_chat_as_read(int chat_id, int except_user_id);

    // Получить последнее сообщение в чате
    std::optional<Message> get_last_in_chat(int chat_id);

    // Количество непрочитанных сообщений в чате
    int get_unread_count(int chat_id, int user_id);

private:
    Database& m_db;

    // Вспомогательный метод для парсинга Message из sqlite3_stmt
    Message parse_message(sqlite3_stmt* stmt);
};