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
        long long birthday = 0);

    // Поиск
    std::optional<User> find_by_login(const std::string& login);
    std::optional<User> find_by_id(int id);
    std::vector<User> find_all();

    // Проверка существования
    bool exists(const std::string& login);

    // Обновление (позже можно добавить)
    // bool update_password(int user_id, const std::string& new_hash);

private:
    Database& m_db;
};