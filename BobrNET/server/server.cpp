#include <iostream>
#include <cstring>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include "../models/User.hpp"
#include "../models/Message.hpp"
#include "../models/MessageStatus.hpp"
#include "../models/Session.hpp"
#include "../models/db/database.hpp"
#include "../models/Platform.hpp"

// Состояния клиента
enum ClientState {
    IDLE,               // обычный режим
    AWAITING_RECIPIENT, // ждём имя получателя
    AWAITING_MESSAGE    // ждём текст сообщения
};

// Структура для хранения данных клиента
struct ClientInfo {
    SOCKET socket;
    std::string name;
    ClientState state;
    std::string pending_recipient;  // временное хранение имени получателя
};

std::map<std::string, ClientInfo> clients;  // имя -> информация о клиенте
std::mutex clients_mutex;

void send_to_client(SOCKET sock, const std::string& msg) {
    send(sock, msg.c_str(), msg.size(), 0);
}

void broadcast(const std::string& msg, SOCKET exclude = INVALID_SOCKET) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto& pair : clients) {
        if (pair.second.socket != exclude) {
            send_to_client(pair.second.socket, msg);
        }
    }
}

// Отправить список онлайн-пользователей
void send_online_list(SOCKET clientSocket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::string list = "=== Online users ===\n";
    for (auto& pair : clients) {
        list += "  - " + pair.first + "\n";
    }
    list += "===================";
    send_to_client(clientSocket, list);
}

void handle_client(SOCKET clientSocket, const std::string& name) {
    char buffer[4096];
    std::cout << "Client '" << name << "' connected" << std::endl;

    // Получаем ссылку на данные клиента
    ClientInfo* clientInfo = nullptr;
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        if (clients.find(name) != clients.end()) {
            clientInfo = &clients[name];
            clientInfo->state = IDLE;
        }
    }

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string msg(buffer);

        // Убираем перевод строки
        if (!msg.empty() && msg.back() == '\n') msg.pop_back();
        if (!msg.empty() && msg.back() == '\r') msg.pop_back();

        std::cout << "Message from " << name << ": " << msg << std::endl;

        // Обновляем указатель (на случай, если структура изменилась)
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            if (clients.find(name) != clients.end()) {
                clientInfo = &clients[name];
            }
            else {
                break; // клиент уже удалён
            }
        }

        // Обработка в зависимости от состояния
        if (clientInfo->state == IDLE) {
            // Обычный режим — обрабатываем команды
            if (msg.rfind("/msg ", 0) == 0) {
                // Быстрый режим: /msg имя текст
                size_t space1 = msg.find(' ', 5);
                if (space1 != std::string::npos) {
                    std::string target = msg.substr(5, space1 - 5);
                    std::string text = msg.substr(space1 + 1);

                    std::lock_guard<std::mutex> lock(clients_mutex);
                    if (clients.find(target) != clients.end()) {
                        std::string full_msg = name + ": " + text;
                        send_to_client(clients[target].socket, full_msg);
                        send_to_client(clientSocket, "[Sent to " + target + "]");
                    }
                    else {
                        send_to_client(clientSocket, "[Error: User '" + target + "' not online]");
                    }
                }
                else {
                    send_to_client(clientSocket, "[Error: Invalid /msg format. Use: /msg username text]");
                }
            }
            else if (msg == "/message") {
                // Интерактивный режим
                send_to_client(clientSocket, "[Enter recipient name (or /cancel to abort)]");
                clientInfo->state = AWAITING_RECIPIENT;
            }
            else if (msg == "/list") {
                send_online_list(clientSocket);
            }
            else if (msg == "/exit") {
                send_to_client(clientSocket, "[Goodbye!]");
                break;
            }
            else {
                send_to_client(clientSocket, "[Unknown command. Commands: /msg name text, /message, /list, /exit]");
            }
        }
        else if (clientInfo->state == AWAITING_RECIPIENT) {
            // Ждём имя получателя
            if (msg == "/cancel") {
                send_to_client(clientSocket, "[Message sending cancelled]");
                clientInfo->state = IDLE;
                clientInfo->pending_recipient.clear();
            }
            else {
                // Проверяем, существует ли такой пользователь
                std::lock_guard<std::mutex> lock(clients_mutex);
                if (clients.find(msg) != clients.end() && msg != name) {
                    clientInfo->pending_recipient = msg;
                    send_to_client(clientSocket, "[Enter your message (or /cancel to abort)]");
                    clientInfo->state = AWAITING_MESSAGE;
                }
                else if (msg == name) {
                    send_to_client(clientSocket, "[Error: You cannot send message to yourself]");
                    // остаёмся в том же состоянии
                }
                else {
                    send_to_client(clientSocket, "[Error: User '" + msg + "' not online. Try again or /cancel]");
                    // остаёмся в том же состоянии
                }
            }
        }
        else if (clientInfo->state == AWAITING_MESSAGE) {
            // Ждём текст сообщения
            if (msg == "/cancel") {
                send_to_client(clientSocket, "[Message sending cancelled]");
                clientInfo->state = IDLE;
                clientInfo->pending_recipient.clear();
            }
            else {
                std::string target = clientInfo->pending_recipient;
                std::lock_guard<std::mutex> lock(clients_mutex);
                if (clients.find(target) != clients.end()) {
                    std::string full_msg = name + ": " + msg;
                    send_to_client(clients[target].socket, full_msg);
                    send_to_client(clientSocket, "[Sent to " + target + "]");
                }
                else {
                    send_to_client(clientSocket, "[Error: User '" + target + "' went offline]");
                }
                clientInfo->state = IDLE;
                clientInfo->pending_recipient.clear();
            }
        }
    }

    // Удаляем клиента из списка
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(name);
    }
    std::cout << "Client '" << name << "' disconnected" << std::endl;
    closesocket(clientSocket);
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    std::cout << "Server started on port 8888" << std::endl;
    std::cout << "Waiting for connections..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        // Получаем имя клиента
        char buffer[256];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            closesocket(clientSocket);
            continue;
        }

        buffer[bytesReceived] = '\0';
        std::string name(buffer);

        if (!name.empty() && name.back() == '\n') name.pop_back();
        if (!name.empty() && name.back() == '\r') name.pop_back();

        // Проверяем, не занято ли имя
        bool name_taken = false;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            if (clients.find(name) != clients.end()) {
                name_taken = true;
            }
            else {
                ClientInfo info;
                info.socket = clientSocket;
                info.name = name;
                info.state = IDLE;
                clients[name] = info;
            }
        }

        if (name_taken) {
            send_to_client(clientSocket, "[Error: Name already taken]");
            closesocket(clientSocket);
            continue;
        }

        send_to_client(clientSocket, "[Welcome " + name + "!]");
        broadcast(name + " joined the chat", clientSocket);

        // Запускаем поток для обработки клиента
        std::thread client_thread(handle_client, clientSocket, name);
        client_thread.detach();
    }

    closesocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}