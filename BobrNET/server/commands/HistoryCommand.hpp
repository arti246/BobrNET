#pragma once

#include "Command.hpp"

class HistoryCommand : public Command {
public:
    void execute(Session& session, const std::string& args) override;
    std::string name() const override { return "/history"; }
    std::string description() const override { return "/history <username> - show chat history with user"; }
};