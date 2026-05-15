#pragma once

#include "Command.hpp"

class OpenCommand : public Command {
public:
    void execute(Session& session, const std::string& args) override;
    std::string name() const override { return "/open"; }
    std::string description() const override { return "/open <chat_id> - open chat and show history"; }
};