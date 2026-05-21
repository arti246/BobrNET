#include "OpenCommand.hpp"
#include "../Session.hpp"
#include "../../models/db/repositories/ChatRepository.hpp";
#include "../../models/db/repositories/LogRepository.hpp";

void OpenCommand::execute(Session& session, const std::string& args) {
    // Проверяем, есть ли аргумент
    if (args.empty()) {
        session.send("[Error: /open <chat_id>]");
        return;
    }

    // Преобразуем аргумент в число
    int chat_id;
    try {
        chat_id = std::stoi(args);
    }
    catch (...) {
        session.send("[Error: Invalid chat ID]");
        return;
    }

    // Проверяем, существует ли чат
    auto chat = session.chats().get_chat_by_id(chat_id);
    if (!chat.has_value()) {
        session.send("[Error: Chat not found]");
        return;
    }

    // Проверяем, является ли пользователь участником чата
    if (!session.chats().is_participant(chat_id, session.user_id())) {
        session.send("[Error: You are not a member of this chat]");
        return;
    }

    // Устанавливаем активный чат
    session.set_active_chat(chat_id);

    // Показываем историю сообщений
    session.send_chat_history(chat_id);

    // Логируем
    session.logs().info("OPEN_CHAT", "User opened chat " + std::to_string(chat_id), session.user_id());
}