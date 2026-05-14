#include "Database.hpp"
#include "../../external/sqlite/sqlite3.h"
#include <iostream>
#include <chrono>

Database::Database(const std::string& path) : m_db(nullptr) {
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

long long Database::current_time() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}