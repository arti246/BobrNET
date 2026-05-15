#include "ChatsCommand.hpp"
#include "../Session.hpp"

void ChatsCommand::execute(Session& session, const std::string&) {
    session.send_chat_list();
}