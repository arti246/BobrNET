#include "Session.hpp"
#include "../models/db/Database.hpp"
#include "../models/db/repositories/UserRepository.hpp"
#include "../models/db/repositories/ChatRepository.hpp"
#include "../models/db/repositories/MessageRepository.hpp"
#include "../models/db/repositories/LogRepository.hpp"
#include "../models/MessageStatus.hpp"
#include "../models/Message.hpp"
#include "../models/Chat.hpp"
#include "../models/User.hpp"

#include "../models/Platform.hpp"
#include <mutex>
#include <map>

// Глобальные репозитории (определены в server.cpp)
extern Database g_db;
extern UserRepository g_users;
extern ChatRepository g_chats;
extern MessageRepository g_messages;
extern LogRepository g_logs;

extern std::map<int, std::shared_ptr<Session>> g_sessions;
extern std::mutex g_sessions_mutex;

int m_active_chat = -1;
static thread_local int t_active_chat = -1;

void Session::set_active_chat(int chat_id) {
    t_active_chat = chat_id;
}

int Session::active_chat() const {
    return t_active_chat;
}

// Вспомогательная функция (объявлена где-то или определим здесь)
std::string get_chat_display_name(int chat_id, int current_user_id);

Session::Session(SOCKET sock, int user_id, const std::string& login)
    : m_socket(sock), m_user_id(user_id), m_login(login) {
}

Session::~Session() = default;

void Session::send(const std::string& msg) {
    std::string message = msg + "\n";
    ::send(m_socket, message.c_str(), message.size(), 0);
}

void Session::send_chat_list() {
    auto user_chats = g_chats.get_user_chats(m_user_id);
    if (user_chats.empty()) {
        send("[No chats yet. Send a message to someone to create a chat]");
        return;
    }

    send("=== Your chats ===");
    for (const auto& chat : user_chats) {
        int unread = g_messages.get_unread_count(chat.id(), m_user_id);
        std::string unread_mark = (unread > 0) ? " (" + std::to_string(unread) + " new)" : "";
        std::string display_name = get_chat_display_name(chat.id(), m_user_id);
        send("  [" + std::to_string(chat.id()) + "] " + display_name + unread_mark);
    }
    send("=================");
}

void Session::send_chat_history(int chat_id) {
    auto opt = g_chats.get_chat_by_id(chat_id);
    if (!opt.has_value()) {
        send("[Error: Chat not found]");
        return;
    }

    const Chat& chat = opt.value();

    if (!g_chats.is_participant(chat_id, m_user_id)) {
        send("[Error: You are not a member of this chat]");
        return;
    }

    auto chat_messages = g_messages.get_by_chat(chat_id, 50);
    if (chat_messages.empty()) {
        send("[No messages in this chat yet]");
        return;
    }

    std::string title = "=== Chat: " + get_chat_display_name(chat_id, m_user_id) + " ===";
    send(title);

    // Выводим сообщения в хронологическом порядке (сначала старые)
    for (auto it = chat_messages.rbegin(); it != chat_messages.rend(); ++it) {
        auto sender = g_users.find_by_id(it->user_id());
        std::string sender_name = sender.has_value() ? sender->login() : "unknown";
        send(sender_name + ": " + it->text());
    }

    send("=========================");

    // Отмечаем сообщения как прочитанные
    g_messages.mark_all_in_chat_as_read(chat_id, m_user_id);
}

void Session::send_message_to_user(const std::string& target_login, const std::string& text) {
    auto target = g_users.find_by_login(target_login);
    if (!target.has_value()) {
        send("[Error: User '" + target_login + "' does not exist]");
        return;
    }

    // Находим или создаём личный чат
    int chat_id = g_chats.find_private_chat(m_user_id, target->id());
    if (chat_id == -1) {
        g_chats.create_private_chat(m_user_id, target->id(), chat_id);
        g_logs.info("CHAT_CREATED", "Private chat created between " + m_login + " and " + target_login, m_user_id);
    }

    // Сохраняем сообщение
    if (!g_messages.save(chat_id, m_user_id, text, MessageStatus::SENT)) {
        send("[Error: Failed to save message]");
        return;
    }

    bool delivered = false;
    {
        std::lock_guard<std::mutex> lock(g_sessions_mutex);  // нужен доступ к g_sessions
        auto it = g_sessions.find(target->id());
        if (it != g_sessions.end()) {
            it->second->send(m_login + ": " + text);
            delivered = true;
            g_messages.mark_all_in_chat_as_delivered(chat_id, target->id());
        }
    }

    if (delivered) {
        send("[Sent to " + target_login + "]");
    }
    else {
        send("[Saved for offline delivery to " + target_login + "]");
    }

    send("[Sent to " + target_login + "]");
    g_logs.info("MESSAGE", m_login + " -> " + target_login + ": " + text, m_user_id);
}

void Session::send_chat_history_with_user(const std::string& username) {
    auto target = g_users.find_by_login(username);
    if (!target.has_value()) {
        send("[Error: User '" + username + "' does not exist]");
        return;
    }

    int chat_id = g_chats.find_private_chat(m_user_id, target->id());
    if (chat_id == -1) {
        send("[No chat history with " + username + "]");
        return;
    }

    send_chat_history(chat_id);
}

void Session::disconnect() {
    send("[Goodbye!]");
}

void Session::send_message_to_active_chat(const std::string& text) {
    int chat_id = active_chat();
    if (chat_id == -1) {
        send("[Error: No active chat. Use /open <chat_id> first]");
        return;
    }

    // Проверяем, всё ли ещё пользователь в чате
    if (!g_chats.is_participant(chat_id, m_user_id)) {
        send("[Error: You are no longer a member of this chat]");
        set_active_chat(-1);
        return;
    }

    // Сохраняем сообщение
    if (!g_messages.save(chat_id, m_user_id, text, MessageStatus::SENT)) {
        send("[Error: Failed to save message]");
        return;
    }

    // Отправляем всем участникам, кроме себя
    auto participants = g_chats.get_participants(chat_id, m_user_id);
    for (const auto& user : participants) {
        auto it = g_sessions.find(user.id());
        if (it != g_sessions.end() && it->second) {
            it->second->send(m_login + ": " + text);
        }
    }

    // Подтверждение отправителю (опционально)
    // send("[Sent]");

    g_logs.info("MESSAGE", m_login + " -> chat " + std::to_string(chat_id) + ": " + text, m_user_id);
}

Database& Session::db() { return g_db; }
UserRepository& Session::users() { return g_users; }
ChatRepository& Session::chats() { return g_chats; }
MessageRepository& Session::messages() { return g_messages; }
LogRepository& Session::logs() { return g_logs; }