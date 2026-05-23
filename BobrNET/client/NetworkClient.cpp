#include "NetworkClient.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include "../models/Platform.hpp"

NetworkClient::NetworkClient()
    : m_sock(INVALID_SOCKET), m_running(false), m_authenticated(false)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

NetworkClient::~NetworkClient()
{
    disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool NetworkClient::connectToServer(const std::string& host, int port)
{
    if (m_sock != INVALID_SOCKET) {
        disconnect();
    }

    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock == INVALID_SOCKET) {
        std::cerr << "[NetworkClient] Socket creation failed" << std::endl;
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    if (connect(m_sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[NetworkClient] Connection failed" << std::endl;
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }

    m_running = true;
    m_receiverThread = std::thread(&NetworkClient::receiveThread, this);
    return true;
}

void NetworkClient::disconnect()
{
    m_running = false;
    if (m_receiverThread.joinable()) {
        m_receiverThread.join();
    }
    if (m_sock != INVALID_SOCKET) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
    m_authenticated = false;
    m_currentUser.clear();
}

void NetworkClient::sendCommand(const std::string& cmd)
{
    if (m_sock != INVALID_SOCKET) {
        std::string toSend = cmd + "\n";
        send(m_sock, toSend.c_str(), toSend.size(), 0);
    }
}

bool NetworkClient::login(const std::string& login, const std::string& password)
{
    std::string auth_cmd = "LOGIN " + login + " " + password;
    sendCommand(auth_cmd);

    // Ждём ответ 2 секунды
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (m_authenticated) {
        m_currentUser = login;
        return true;
    }
    return false;
}

bool NetworkClient::registerUser(const std::string& login, const std::string& password, const std::string& birthday)
{
    std::string auth_cmd = "REGISTER " + login + " " + password + " " + birthday;
    sendCommand(auth_cmd);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (m_authenticated) {
        m_currentUser = login;
        return true;
    }
    return false;
}

void NetworkClient::receiveThread()
{
    char buffer[4096];
    while (m_running) {
        int bytes = recv(m_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            if (m_running) {
                std::cerr << "[NetworkClient] Connection lost" << std::endl;
                if (m_callback) m_callback("[Disconnected from server]");
            }
            break;
        }
        buffer[bytes] = '\0';
        std::string msg(buffer);

        // Убираем лишние переводы строк
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }

        if (!msg.empty()) {
            // Проверяем успешность аутентификации
            if (msg.find("successful") != std::string::npos ||
                msg.find("Welcome back") != std::string::npos ||
                msg.find("Registration successful") != std::string::npos) {
                m_authenticated = true;
            }

            // Передаём все сообщения в callback
            if (m_callback) {
                m_callback(msg);
            }
        }
    }
}

void NetworkClient::setMessageCallback(std::function<void(const std::string&)> callback)
{
    m_callback = callback;
}

bool NetworkClient::isConnected() const
{
    return m_sock != INVALID_SOCKET && m_running;
}

std::string NetworkClient::getCurrentUser() const
{
    return m_currentUser;
}

void NetworkClient::clear()
{
    disconnect();
}