#include "LogRepository.hpp"
#include "../Database.hpp"
#include "../../../external/sqlite/sqlite3.h"
#include <iostream>

LogRepository::LogRepository(Database& db) : m_db(db) {}

bool LogRepository::log(const std::string& level, const std::string& event_type,
    const std::string& message, int user_id) {
    const char* sql = "INSERT INTO logs (timestamp, level, event_type, message, user_id) "
        "VALUES (?, ?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare log insert: " << sqlite3_errmsg(m_db.handle()) << std::endl;
        return false;
    }

    long long now = Database::current_time();

    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, level.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, event_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, message.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, user_id);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

bool LogRepository::info(const std::string& event_type, const std::string& message, int user_id) {
    return log("INFO", event_type, message, user_id);
}

bool LogRepository::warning(const std::string& event_type, const std::string& message, int user_id) {
    return log("WARNING", event_type, message, user_id);
}

bool LogRepository::error(const std::string& event_type, const std::string& message, int user_id) {
    return log("ERROR", event_type, message, user_id);
}

std::vector<LogEntry> LogRepository::get_recent(int limit) {
    std::vector<LogEntry> entries;
    const char* sql = "SELECT timestamp, level, event_type, message, user_id "
        "FROM logs ORDER BY timestamp DESC LIMIT ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return entries;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LogEntry entry;
        entry.timestamp = sqlite3_column_int64(stmt, 0);
        entry.level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        entry.event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        entry.message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        entry.user_id = sqlite3_column_int(stmt, 4);
        entries.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return entries;
}

std::vector<LogEntry> LogRepository::get_by_user(int user_id, int limit) {
    std::vector<LogEntry> entries;
    const char* sql = "SELECT timestamp, level, event_type, message, user_id "
        "FROM logs WHERE user_id = ? ORDER BY timestamp DESC LIMIT ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return entries;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LogEntry entry;
        entry.timestamp = sqlite3_column_int64(stmt, 0);
        entry.level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        entry.event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        entry.message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        entry.user_id = sqlite3_column_int(stmt, 4);
        entries.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return entries;
}

std::vector<LogEntry> LogRepository::get_by_level(const std::string& level, int limit) {
    std::vector<LogEntry> entries;
    const char* sql = "SELECT timestamp, level, event_type, message, user_id "
        "FROM logs WHERE level = ? ORDER BY timestamp DESC LIMIT ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return entries;
    }

    sqlite3_bind_text(stmt, 1, level.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LogEntry entry;
        entry.timestamp = sqlite3_column_int64(stmt, 0);
        entry.level = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        entry.event_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        entry.message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        entry.user_id = sqlite3_column_int(stmt, 4);
        entries.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return entries;
}

bool LogRepository::delete_older_than(long long timestamp) {
    const char* sql = "DELETE FROM logs WHERE timestamp < ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, timestamp);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}