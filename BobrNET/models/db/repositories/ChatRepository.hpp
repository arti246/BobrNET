#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../../Chat.hpp";

class Database;
class Chat;
class User;

class ChatRepository {
public:
    explicit ChatRepository(Database& db);

    // Создание чатов
    bool create_private_chat(int user1_id, int user2_id, int& out_chat_id);
    bool create_group_chat(const std::string& name, const std::vector<int>& user_ids, int& out_chat_id);

    // Получение чатов пользователя
    std::vector<Chat> get_user_chats(int user_id);

    // Получение участников чата
    std::vector<User> get_participants(int chat_id, int exclude_user_id = -1);
    std::vector<int> get_participant_ids(int chat_id);

    // Поиск личного чата между двумя пользователями
    int find_private_chat(int user1_id, int user2_id);  // вернёт chat_id или -1

    // Проверка, является ли пользователь участником чата
    bool is_participant(int chat_id, int user_id);

    // Добавление участника в групповой чат
    bool add_participant(int chat_id, int user_id);

    // Удаление участника из чата
    bool remove_participant(int chat_id, int user_id);

    // Получение чата по ID
    std::optional<Chat> get_chat_by_id(int chat_id);

private:
    Database& m_db;
};