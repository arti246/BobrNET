#pragma once

#include <string>

class Session;

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(Session& session, const std::string& args) = 0;
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
};