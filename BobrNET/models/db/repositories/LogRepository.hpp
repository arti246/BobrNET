#pragma once

#include <string>
#include <vector>

class Database;

struct LogEntry {
    long long timestamp;
    std::string level;
    std::string event_type;
    std::string message;
    int user_id;
};

class LogRepository {
public:
    explicit LogRepository(Database& db);

    // Запись события
    bool log(const std::string& level, const std::string& event_type,
        const std::string& message, int user_id = -1);

    // Удобные методы-обёртки
    bool info(const std::string& event_type, const std::string& message, int user_id = -1);
    bool warning(const std::string& event_type, const std::string& message, int user_id = -1);
    bool error(const std::string& event_type, const std::string& message, int user_id = -1);

    // Получение логов (для администрирования)
    std::vector<LogEntry> get_recent(int limit = 100);
    std::vector<LogEntry> get_by_user(int user_id, int limit = 50);
    std::vector<LogEntry> get_by_level(const std::string& level, int limit = 50);

    // Очистка старых логов (опционально)
    bool delete_older_than(long long timestamp);

private:
    Database& m_db;
};