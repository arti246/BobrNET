#include "MessageRepository.hpp"
#include "../Database.hpp"
#include "../../Message.hpp"
#include "../../../external/sqlite/sqlite3.h"
#include <iostream>

MessageRepository::MessageRepository(Database& db) : m_db(db) {}

Message MessageRepository::parse_message(sqlite3_stmt* stmt) {
    return Message(
        sqlite3_column_int(stmt, 0),   // id
        sqlite3_column_int(stmt, 1),   // chat_id
        sqlite3_column_int(stmt, 2),   // user_id
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)), // text
        sqlite3_column_int64(stmt, 4), // timestamp
        sqlite3_column_int(stmt, 5)    // status
    );
}

bool MessageRepository::save(int chat_id, int from_user_id, const std::string& text, int status) {
    const char* sql = "INSERT INTO messages (chat_id, user_id, text, timestamp, status) "
        "VALUES (?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare message insert" << std::endl;
        return false;
    }

    long long now = Database::current_time();

    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, from_user_id);
    sqlite3_bind_text(stmt, 3, text.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, now);
    sqlite3_bind_int(stmt, 5, status);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

bool MessageRepository::save(const Message& message) {
    return save(message.chat_id(), message.user_id(), message.text(), message.status());
}

std::vector<Message> MessageRepository::get_by_chat(int chat_id, int limit, int offset) {
    std::vector<Message> messages;
    const char* sql = R"(
        SELECT id, chat_id, user_id, text, timestamp, status
        FROM messages
        WHERE chat_id = ?
        ORDER BY timestamp DESC
        LIMIT ? OFFSET ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        messages.push_back(parse_message(stmt));
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::vector<Message> MessageRepository::get_after_timestamp(int chat_id, long long timestamp, int limit) {
    std::vector<Message> messages;
    const char* sql = R"(
        SELECT id, chat_id, user_id, text, timestamp, status
        FROM messages
        WHERE chat_id = ? AND timestamp > ?
        ORDER BY timestamp ASC
        LIMIT ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int64(stmt, 2, timestamp);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        messages.push_back(parse_message(stmt));
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::vector<Message> MessageRepository::get_undelivered(int user_id) {
    std::vector<Message> messages;
    const char* sql = R"(
        SELECT m.id, m.chat_id, m.user_id, m.text, m.timestamp, m.status
        FROM messages m
        JOIN chat_participants cp ON m.chat_id = cp.chat_id
        WHERE cp.user_id = ? AND m.user_id != ? AND m.status = 0
        ORDER BY m.timestamp ASC
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, user_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        messages.push_back(parse_message(stmt));
    }

    sqlite3_finalize(stmt);
    return messages;
}

bool MessageRepository::update_status(int message_id, int status) {
    const char* sql = "UPDATE messages SET status = ? WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_int(stmt, 2, message_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

bool MessageRepository::mark_as_delivered(int message_id) {
    return update_status(message_id, 1);  // 1 = DELIVERED
}

bool MessageRepository::mark_as_read(int message_id) {
    return update_status(message_id, 2);  // 2 = READ
}

std::vector<Message> MessageRepository::get_between_users(int user1_id, int user2_id, int limit) {
    // Сначала находим общий чат
    const char* find_chat_sql = R"(
        SELECT c.id
        FROM chats c
        JOIN chat_participants cp1 ON c.id = cp1.chat_id
        JOIN chat_participants cp2 ON c.id = cp2.chat_id
        WHERE c.type = 'private'
          AND cp1.user_id = ?
          AND cp2.user_id = ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), find_chat_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }

    sqlite3_bind_int(stmt, 1, user1_id);
    sqlite3_bind_int(stmt, 2, user2_id);

    int chat_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        chat_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (chat_id == -1) {
        return {};  // Чат не найден
    }

    return get_by_chat(chat_id, limit);
}

bool MessageRepository::mark_all_in_chat_as_delivered(int chat_id, int except_user_id) {
    const char* sql = R"(
        UPDATE messages 
        SET status = 1 
        WHERE chat_id = ? AND user_id != ? AND status = 0
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, except_user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

bool MessageRepository::mark_all_in_chat_as_read(int chat_id, int except_user_id) {
    const char* sql = R"(
        UPDATE messages 
        SET status = 2 
        WHERE chat_id = ? AND user_id != ? AND status < 2
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, except_user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

std::optional<Message> MessageRepository::get_last_in_chat(int chat_id) {
    const char* sql = R"(
        SELECT id, chat_id, user_id, text, timestamp, status
        FROM messages
        WHERE chat_id = ?
        ORDER BY timestamp DESC
        LIMIT 1
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, chat_id);

    std::optional<Message> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = parse_message(stmt);
    }

    sqlite3_finalize(stmt);
    return result;
}

int MessageRepository::get_unread_count(int chat_id, int user_id) {
    const char* sql = R"(
        SELECT COUNT(*)
        FROM messages
        WHERE chat_id = ? AND user_id != ? AND status < 2
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, user_id);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}