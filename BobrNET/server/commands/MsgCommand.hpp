#pragma once

#include "Command.hpp"

class MsgCommand : public Command {
public:
    void execute(Session& session, const std::string& args) override;
    std::string name() const override { return "/msg"; }
    std::string description() const override { return "/msg <user> <text> - send message"; }
};