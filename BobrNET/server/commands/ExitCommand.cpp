#include "ExitCommand.hpp"
#include "../Session.hpp"

void ExitCommand::execute(Session& session, const std::string&) {
    session.disconnect();
}