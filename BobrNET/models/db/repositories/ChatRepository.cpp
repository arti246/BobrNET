#include "ChatRepository.hpp"
#include "../Database.hpp"
#include "../../Chat.hpp"
#include "../../User.hpp"
#include "../../../external/sqlite/sqlite3.h"
#include <iostream>

ChatRepository::ChatRepository(Database& db) : m_db(db) {}

bool ChatRepository::create_private_chat(int user1_id, int user2_id, int& out_chat_id) {
    // Проверяем, не существует ли уже чат между этими пользователями
    int existing_chat = find_private_chat(user1_id, user2_id);
    if (existing_chat != -1) {
        out_chat_id = existing_chat;
        return true;  // Чат уже существует
    }

    // Создаём новый чат
    const char* sql = "INSERT INTO chats (type, name, created_at) VALUES ('private', NULL, ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare chat insert" << std::endl;
        return false;
    }

    long long now = Database::current_time();
    sqlite3_bind_int64(stmt, 1, now);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    out_chat_id = sqlite3_last_insert_rowid(m_db.handle());
    sqlite3_finalize(stmt);

    // Добавляем участников
    if (!add_participant(out_chat_id, user1_id)) return false;
    if (!add_participant(out_chat_id, user2_id)) return false;

    return true;
}

bool ChatRepository::create_group_chat(const std::string& name, const std::vector<int>& user_ids, int& out_chat_id) {
    if (user_ids.size() < 2) {
        std::cerr << "Group chat must have at least 2 participants" << std::endl;
        return false;
    }

    // Создаём групповой чат
    const char* sql = "INSERT INTO chats (type, name, created_at) VALUES ('group', ?, ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare group chat insert" << std::endl;
        return false;
    }

    long long now = Database::current_time();
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, now);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    out_chat_id = sqlite3_last_insert_rowid(m_db.handle());
    sqlite3_finalize(stmt);

    // Добавляем всех участников
    for (int user_id : user_ids) {
        if (!add_participant(out_chat_id, user_id)) {
            return false;
        }
    }

    return true;
}

std::vector<Chat> ChatRepository::get_user_chats(int user_id) {
    std::vector<Chat> chats;
    const char* sql = R"(
        SELECT c.id, c.type, c.name, c.created_at
        FROM chats c
        JOIN chat_participants cp ON c.id = cp.chat_id
        WHERE cp.user_id = ?
        ORDER BY (
            SELECT MAX(m.timestamp) FROM messages m WHERE m.chat_id = c.id
        ) DESC
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return chats;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        chats.emplace_back(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            sqlite3_column_text(stmt, 2) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "",
            sqlite3_column_int64(stmt, 3)
        );
    }

    sqlite3_finalize(stmt);
    return chats;
}

int ChatRepository::find_private_chat(int user1_id, int user2_id) {
    const char* sql = R"(
        SELECT c.id
        FROM chats c
        JOIN chat_participants cp1 ON c.id = cp1.chat_id
        JOIN chat_participants cp2 ON c.id = cp2.chat_id
        WHERE c.type = 'private'
          AND cp1.user_id = ?
          AND cp2.user_id = ?
          AND cp1.user_id != cp2.user_id
        LIMIT 1
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user1_id);
    sqlite3_bind_int(stmt, 2, user2_id);

    int chat_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        chat_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return chat_id;
}

std::vector<User> ChatRepository::get_participants(int chat_id, int exclude_user_id) {
    std::vector<User> users;
    const char* sql = R"(
        SELECT u.id, u.login, u.password_hash, u.created_at, u.birthday
        FROM users u
        JOIN chat_participants cp ON u.id = cp.user_id
        WHERE cp.chat_id = ?
    )";

    if (exclude_user_id != -1) {
        sql = R"(
            SELECT u.id, u.login, u.password_hash, u.created_at, u.birthday
            FROM users u
            JOIN chat_participants cp ON u.id = cp.user_id
            WHERE cp.chat_id = ? AND u.id != ?
        )";
    }

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return users;
    }

    sqlite3_bind_int(stmt, 1, chat_id);
    if (exclude_user_id != -1) {
        sqlite3_bind_int(stmt, 2, exclude_user_id);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        users.emplace_back(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            sqlite3_column_int64(stmt, 3),
            sqlite3_column_int64(stmt, 4)
        );
    }

    sqlite3_finalize(stmt);
    return users;
}

std::vector<int> ChatRepository::get_participant_ids(int chat_id) {
    std::vector<int> ids;
    const char* sql = "SELECT user_id FROM chat_participants WHERE chat_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return ids;
    }

    sqlite3_bind_int(stmt, 1, chat_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.push_back(sqlite3_column_int(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return ids;
}

bool ChatRepository::add_participant(int chat_id, int user_id) {
    const char* sql = "INSERT INTO chat_participants (chat_id, user_id, joined_at) VALUES (?, ?, ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    long long now = Database::current_time();
    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int64(stmt, 3, now);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

bool ChatRepository::remove_participant(int chat_id, int user_id) {
    const char* sql = "DELETE FROM chat_participants WHERE chat_id = ? AND user_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

bool ChatRepository::is_participant(int chat_id, int user_id) {
    const char* sql = "SELECT 1 FROM chat_participants WHERE chat_id = ? AND user_id = ? LIMIT 1";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, chat_id);
    sqlite3_bind_int(stmt, 2, user_id);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    return exists;
}

std::optional<Chat> ChatRepository::get_chat_by_id(int chat_id) {
    const char* sql = "SELECT id, type, name, created_at FROM chats WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, chat_id);

    std::optional<Chat> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = Chat(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            sqlite3_column_text(stmt, 2) ? reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) : "",
            sqlite3_column_int64(stmt, 3)
        );
    }

    sqlite3_finalize(stmt);
    return result;
}