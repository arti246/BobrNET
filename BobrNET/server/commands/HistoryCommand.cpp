#include "HistoryCommand.hpp"
#include "../Session.hpp"

void HistoryCommand::execute(Session& session, const std::string& args) {
    if (args.empty()) {
        session.send("[Error: /history <username>]");
        return;
    }

    session.send_chat_history_with_user(args);
}