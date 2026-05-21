#include "UserRepository.hpp"
#include "../Database.hpp"
#include "../../User.hpp"
#include "../../../external/sqlite/sqlite3.h"
#include <iostream>

UserRepository::UserRepository(Database& db) : m_db(db) {}

bool UserRepository::create(const std::string& login, const std::string& password_hash,
    const std::string& birthday) {
    const char* sql = "INSERT INTO users (login, password_hash, created_at, birthday) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_db.handle()) << std::endl;
        return false;
    }

    long long now_Created_at = Database::current_time();
    long long nowBirthday = Database::current_time();

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, now_Created_at);
    sqlite3_bind_int64(stmt, 4, nowBirthday);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success;
}

std::optional<User> UserRepository::find_by_login(const std::string& login) {
    const char* sql = "SELECT id, login, password_hash, created_at, birthday FROM users WHERE login = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = User(
            sqlite3_column_int(stmt, 0),                                           // id
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),          // login
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),          // password_hash
            sqlite3_column_int64(stmt, 3),                                        // created_at
            sqlite3_column_int64(stmt, 4)                                         // birthday
        );
    }

    sqlite3_finalize(stmt);
    return result;
}

std::optional<User> UserRepository::find_by_id(int id) {
    const char* sql = "SELECT id, login, password_hash, created_at, birthday FROM users WHERE id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);

    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = User(
            sqlite3_column_int(stmt, 0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
            sqlite3_column_int64(stmt, 3),
            sqlite3_column_int64(stmt, 4)
        );
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<User> UserRepository::find_all() {
    std::vector<User> users;
    const char* sql = "SELECT id, login, password_hash, created_at, birthday FROM users ORDER BY login";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return users;
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

bool UserRepository::exists(const std::string& login) {
    const char* sql = "SELECT 1 FROM users WHERE login = ? LIMIT 1";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    return exists;
}