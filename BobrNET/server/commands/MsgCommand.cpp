#include "MsgCommand.hpp"
#include "../Session.hpp"

void MsgCommand::execute(Session& session, const std::string& args) {
    size_t space = args.find(' ');
    if (space == std::string::npos) {
        session.send("[Error: /msg username text]");
        return;
    }

    std::string target = args.substr(0, space);
    std::string text = args.substr(space + 1);

    session.send_message_to_user(target, text);
}