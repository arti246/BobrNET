#pragma once

#include <string>
#include <vector>

class Chat {
public:
    Chat() = default;
    Chat(int id, const std::string& type, const std::string& name, long long created_at)
        : m_id(id), m_type(type), m_name(name), m_created_at(created_at) {
    }

    int id() const { return m_id; }
    const std::string& type() const { return m_type; }
    const std::string& name() const { return m_name; }
    long long created_at() const { return m_created_at; }

    bool is_private() const { return m_type == "private"; }
    bool is_group() const { return m_type == "group"; }

private:
    int m_id = -1;
    std::string m_type;
    std::string m_name;
    long long m_created_at = 0;
};