#pragma once

#include "MessageStatus.hpp"
#include <string>

class Message {
public:
    Message() = default;
    Message(int id, int from_id, int to_id, const std::string& text,
        long long timestamp, MessageStatus status)
        : m_id(id), m_from_id(from_id), m_to_id(to_id), m_text(text),
        m_timestamp(timestamp), m_status(status) {
    }

    int id() const { return m_id; }
    int from_id() const { return m_from_id; }
    int to_id() const { return m_to_id; }
    int chat_id() const { return m_chat_id; }
    const std::string& text() const { return m_text; }
    long long timestamp() const { return m_timestamp; }
    MessageStatus status() const { return m_status; }

    void set_status(MessageStatus status) { m_status = status; }

private:
    int m_id = -1;
    int m_from_id = -1;
    int m_to_id = -1;
    std::string m_text;
    long long m_timestamp = 0;
    MessageStatus m_status = MessageStatus::SENT;
    int m_chat_id;
};