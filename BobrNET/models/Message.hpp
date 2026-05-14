#pragma once

#include "MessageStatus.hpp"
#include <string>

class Message {
public:
    Message() = default;
    Message(int id, int chat_id, int user_id, const std::string& text,
        long long timestamp, int status)
        : m_id(id), m_chat_id(chat_id), m_user_id(user_id),
        m_text(text), m_timestamp(timestamp), m_status(status) {
    }

    int id() const { return m_id; }
    int chat_id() const { return m_chat_id; }
    int user_id() const { return m_user_id; }
    const std::string& text() const { return m_text; }
    long long timestamp() const { return m_timestamp; }
    int status() const { return m_status; }

    void set_status(int status) { m_status = status; }

private:
    int m_id = -1;
    int m_chat_id = -1;
    int m_user_id = -1;
    std::string m_text;
    long long m_timestamp = 0;
    int m_status = 0;  // 0=SENT, 1=DELIVERED, 2=READ
};