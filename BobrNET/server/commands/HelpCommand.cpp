#include "HelpCommand.hpp"
#include "../Session.hpp"
#include "CommandDispatcher.hpp"

void HelpCommand::execute(Session& session, const std::string&) {
    CommandDispatcher dispatcher;
    dispatcher.print_help(session);
}