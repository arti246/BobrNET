#pragma once

#include <string>
#include <optional>
#include <vector>

class Database;
class User;

class UserRepository {
public:
    explicit UserRepository(Database& db);

    // Создание пользователя
    bool create(const std::string& login, const std::string& password_hash,
        const std::string& birthday);

    // Поиск
    std::optional<User> find_by_login(const std::string& login);
    std::optional<User> find_by_id(int id);
    std::vector<User> find_all();

    // Проверка существования
    bool exists(const std::string& login);

private:
    Database& m_db;
};