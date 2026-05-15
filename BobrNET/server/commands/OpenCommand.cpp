#include "OpenCommand.hpp"
#include "../Session.hpp"

void OpenCommand::execute(Session& session, const std::string& args) {
    if (args.empty()) {
        session.send("[Error: /open <chat_id>]");
        return;
    }

    try {
        int chat_id = std::stoi(args);
        session.send_chat_history(chat_id);
    }
    catch (...) {
        session.send("[Error: Invalid chat ID]");
    }
}