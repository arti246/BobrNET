#pragma once

#include "Platform.hpp"
#include <string>
#include <chrono>
#include "User.hpp"

class Session {
public:
    Session(const User& user, SOCKET socket, const std::string& ip)
        : m_user(user), m_socket(socket), m_ip(ip), m_connected_at(get_current_time()) {
    }

    const User& user() const { return m_user; }
    SOCKET socket() const { return m_socket; }
    const std::string& ip() const { return m_ip; }
    long long connected_at() const { return m_connected_at; }

private:
    User m_user;
    SOCKET m_socket;
    std::string m_ip;
    long long m_connected_at;

    static long long get_current_time() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};