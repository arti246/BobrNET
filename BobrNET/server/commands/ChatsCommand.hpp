#pragma once

#include "Command.hpp"

class ChatsCommand : public Command {
public:
    void execute(Session& session, const std::string& args) override;
    std::string name() const override { return "/chats"; }
    std::string description() const override { return "/chats - list your chats"; }
};