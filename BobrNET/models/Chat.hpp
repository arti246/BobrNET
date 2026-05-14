#pragma once

#include <string>
#include <vector>

class Chat {
public:
    Chat(int id, const std::string& type, const std::string& name, long long created_at);

    int id() const;
    const std::string& type() const {return m_type};
    const std::string& name() const {return m_name};
    long long created_at() const {return m_created_at};

private:
    int m_id;
    std::string m_type;   // "private" или "group"
    std::string m_name;   // название (для групп)
    long long m_created_at;
};