#pragma once

class ChatParticipant {
public:
    ChatParticipant(int chat_id, int user_id, long long joined_at);

    int chat_id() const;
    int user_id() const;
    long long joined_at() const;

private:
    int m_chat_id;
    int m_user_id;
    long long m_joined_at;
};