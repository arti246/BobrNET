#pragma once

#include <string>
#include <functional>

class NetworkClient {
public:
    NetworkClient() = default;
    bool connectToServer(const std::string&, int) { return true; }
    void sendCommand(const std::string&) {}
    void setMessageCallback(std::function<void(const std::string&)>) {}
    bool login(const std::string&, const std::string&) { return true; }
    bool registerUser(const std::string&, const std::string&, const std::string&) { return true; }
    void disconnect() {}
};