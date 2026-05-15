#include "CommandDispatcher.hpp"
#include "Command.hpp"
#include "MsgCommand.hpp"
#include "ChatsCommand.hpp"
#include "OpenCommand.hpp"
#include "HistoryCommand.hpp"
#include "HelpCommand.hpp"
#include "ExitCommand.hpp"
#include "../Session.hpp"

CommandDispatcher::CommandDispatcher() {
    register_command(std::make_unique<MsgCommand>());
    register_command(std::make_unique<ChatsCommand>());
    register_command(std::make_unique<OpenCommand>());
    register_command(std::make_unique<HistoryCommand>());
    register_command(std::make_unique<HelpCommand>());
    register_command(std::make_unique<ExitCommand>());
}

CommandDispatcher::~CommandDispatcher() = default;

void CommandDispatcher::register_command(std::unique_ptr<Command> cmd) {
    m_commands[cmd->name()] = std::move(cmd);
}

bool CommandDispatcher::dispatch(Session& session, const std::string& cmd_line) {
    size_t space = cmd_line.find(' ');
    std::string cmd_name = (space != std::string::npos) ? cmd_line.substr(0, space) : cmd_line;
    std::string args = (space != std::string::npos) ? cmd_line.substr(space + 1) : "";

    auto it = m_commands.find(cmd_name);
    if (it != m_commands.end()) {
        it->second->execute(session, args);
        return true;
    }
    return false;
}

void CommandDispatcher::print_help(Session& session) {
    session.send("=== Commands ===");
    for (const auto& [name, cmd] : m_commands) {
        session.send("  " + cmd->description());
    }
    session.send("=================");
}