#pragma once

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <future>

class NetworkClient
{
public:
    NetworkClient();
    ~NetworkClient();

    // Подключение
    bool connectToServer(const std::string& host, int port);
    void disconnect();

    // Аутентификация (теперь возвращает реальный результат)
    bool login(const std::string& login, const std::string& password);
    bool registerUser(const std::string& login, const std::string& password, const std::string& birthday);

    // Отправка команд
    void sendCommand(const std::string& cmd);

    // Состояние
    bool isConnected() const;
    std::string getCurrentUser() const;

    // Callback для получения сообщений от сервера
    void setMessageCallback(std::function<void(const std::string&)> callback);

    void clear();

private:
    void receiveThread();
    bool authenticate(const std::string& action, const std::string& login,
        const std::string& password, const std::string& birthday = "");

    int m_sock;
    std::thread m_receiverThread;
    bool m_running;
    bool m_authenticated;
    std::string m_currentUser;
    std::mutex m_mutex;
    std::function<void(const std::string&)> m_callback;

    // Для ожидания ответа аутентификации
    std::promise<bool> m_authPromise;
    bool m_waitingForAuth;
};