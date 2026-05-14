#pragma once

#include <string>

struct sqlite3;

class Database {
public:
    Database(const std::string& path);
    ~Database();

    sqlite3* handle() const { return m_db; }

    // Выполнить SQL (для создания таблиц)
    bool execute_sql(const std::string& sql);

    // Вспомогательные методы
    static long long current_time();

    bool Database::init();

private:
    sqlite3* m_db;
};