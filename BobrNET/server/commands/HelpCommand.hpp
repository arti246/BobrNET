#pragma once

#include "Command.hpp"

class HelpCommand : public Command {
public:
    void execute(Session& session, const std::string& args) override;
    std::string name() const override { return "/help"; }
    std::string description() const override { return "/help - show this help"; }
};