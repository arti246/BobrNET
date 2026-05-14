#pragma once

#include <string>

class User {
public:
    User() = default;
    User(int id, const std::string& login, const std::string& password_hash,
        long long created_at = 0, long long birthday = 0)
        : m_id(id), m_login(login), m_password_hash(password_hash),
        m_created_at(created_at), m_birthday(birthday) {
    }

    int id() const { return m_id; }
    const std::string& login() const { return m_login; }
    const std::string& password_hash() const { return m_password_hash; }
    long long created_at() const { return m_created_at; }
    long long birthday() const { return m_birthday; }

private:
    int m_id = -1;
    std::string m_login;
    std::string m_password_hash;
    long long m_created_at = 0;
    long long m_birthday = 0;
};