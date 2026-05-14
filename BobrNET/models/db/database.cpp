#include "database.hpp"
#include "../User.hpp"
#include "../Message.hpp"
#include <iostream>
#include <chrono>
#include <cstdio>

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(m_db) << std::endl;
        m_db = nullptr;
    }
}

Database::~Database() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

bool Database::init() {
    const char* sql = R"(
        -- Таблица пользователей
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            login TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            created_at INTEGER NOT NULL,
            birthday INTEGER
        );
        
        -- Таблица чатов
        CREATE TABLE IF NOT EXISTS chats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            type TEXT NOT NULL CHECK(type IN ('private', 'group')),
            name TEXT,
            created_at INTEGER NOT NULL
        );
        
        -- Таблица участников чатов
        CREATE TABLE IF NOT EXISTS chat_participants (
            chat_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            joined_at INTEGER NOT NULL,
            PRIMARY KEY (chat_id, user_id),
            FOREIGN KEY (chat_id) REFERENCES chats(id) ON DELETE CASCADE,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
        );
        
        -- Таблица сообщений
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            chat_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            text TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            status INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY (chat_id) REFERENCES chats(id) ON DELETE CASCADE,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
        );
        
        -- Таблица логов
        CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            level TEXT NOT NULL,
            event_type TEXT NOT NULL,
            message TEXT NOT NULL,
            user_id INTEGER DEFAULT -1
        );
        
        -- Индексы для быстрого поиска
        CREATE INDEX IF NOT EXISTS idx_messages_chat ON messages(chat_id);
        CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages(timestamp);
        CREATE INDEX IF NOT EXISTS idx_messages_status ON messages(status);
        CREATE INDEX IF NOT EXISTS idx_chat_participants_user ON chat_participants(user_id);
        CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON logs(timestamp);
    )";

    return execute_sql(sql);
}

bool Database::execute_sql(const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::create_user(const std::string& login, const std::string& password_hash) {
    long long now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string sql = "INSERT INTO users (login, password_hash, created_at) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::optional<User> Database::find_user_by_login(const std::string& login) {
    std::string sql = "SELECT id, login, password_hash FROM users WHERE login = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    User user;
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = User(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))
        );
        found = true;
    }

    sqlite3_finalize(stmt);
    return found ? std::optional<User>(user) : std::nullopt;
}

std::optional<User> Database::find_user_by_id(const int id) {
    std::string sql = "SELECT id, login, password_hash FROM users WHERE id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    char buffer[10];

    sqlite3_bind_text(stmt, 1, std::to_string(id).c_str(), -1, SQLITE_STATIC);

    User user;
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = User(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))
        );
        found = true;
    }

    sqlite3_finalize(stmt);
    return found ? std::optional<User>(user) : std::nullopt;
}

bool Database::save_message(int from_id, int to_id, const std::string& text, MessageStatus status) {
    long long now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    std::string sql = "INSERT INTO messages (from_id, to_id, text, timestamp, status) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, from_id);
    sqlite3_bind_int(stmt, 2, to_id);
    sqlite3_bind_text(stmt, 3, text.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, now);
    sqlite3_bind_int(stmt, 5, static_cast<int>(status));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<Message> Database::get_messages_between(int user1_id, int user2_id, int limit) {
    std::vector<Message> messages;
    std::string sql = R"(
        SELECT id, from_id, to_id, text, timestamp, status 
        FROM messages 
        WHERE (from_id = ? AND to_id = ?) OR (from_id = ? AND to_id = ?)
        ORDER BY timestamp DESC
        LIMIT ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, user1_id);
    sqlite3_bind_int(stmt, 2, user2_id);
    sqlite3_bind_int(stmt, 3, user2_id);
    sqlite3_bind_int(stmt, 4, user1_id);
    sqlite3_bind_int(stmt, 5, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message msg(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_int(stmt, 2),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            sqlite3_column_int64(stmt, 4),
            static_cast<MessageStatus>(sqlite3_column_int(stmt, 5))
        );
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::vector<Message> Database::get_undelivered_messages(int user_id) {
    std::vector<Message> messages;
    std::string sql = R"(
        SELECT id, from_id, to_id, text, timestamp, status 
        FROM messages 
        WHERE to_id = ? AND status = 0
        ORDER BY timestamp ASC
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return messages;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message msg(
            sqlite3_column_int(stmt, 0),
            sqlite3_column_int(stmt, 1),
            sqlite3_column_int(stmt, 2),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)),
            sqlite3_column_int64(stmt, 4),
            static_cast<MessageStatus>(sqlite3_column_int(stmt, 5))
        );
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}