#pragma once

#include <map>
#include <memory>
#include <string>

class Command;
class Session;

class CommandDispatcher {
public:
    CommandDispatcher();
    ~CommandDispatcher();

    void register_command(std::unique_ptr<Command> cmd);
    bool dispatch(Session& session, const std::string& cmd_line);

    void print_help(Session& session);

private:
    std::map<std::string, std::unique_ptr<Command>> m_commands;
};