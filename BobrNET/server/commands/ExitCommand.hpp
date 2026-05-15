#pragma once

#include "Command.hpp"

class ExitCommand : public Command {
public:
    void execute(Session& session, const std::string& args) override;
    std::string name() const override { return "/exit"; }
    std::string description() const override { return "/exit - disconnect"; }
};