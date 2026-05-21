#include <iostream>
#include <cstring>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

SOCKET sock;
bool connected = true;
bool in_chat = false;

void receive_messages() {
    char buffer[4096];
    while (connected) {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            connected = false;
            break;
        }
        buffer[bytes] = '\0';
        std::string msg(buffer);

        // Убираем лишние переводы строк
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }

        if (msg.empty()) continue;

        // Проверяем, является ли сообщение системным
        bool is_system = (msg[0] == '[') ||
            (msg.find("===") == 0) ||
            (msg.find("=================") == 0) ||
            (msg.find("Unknown command") == 0) ||
            (msg.find("[Error") == 0) ||
            (msg.find("[Sent to") == 0) ||
            (msg.find("[Welcome") == 0) ||
            (msg.find("[No chats yet") == 0);

        // Стираем текущую строку (если есть символы)
        // Для Windows и Linux/Unix
        std::cout << "\r";  // возврат в начало строки
        std::cout << "\033[K"; // очистка строки (работает в терминалах, поддерживающих ANSI)

        // Выводим сообщение
        if (in_chat && !is_system && msg.find("=== Chat:") != 0 && msg.find("[Exited chat]") != 0) {
            // Это сообщение от другого пользователя
            std::cout << msg << std::endl;
        }
        else {
            // Системное сообщение или список чатов
            std::cout << msg << std::endl;
        }

        // Восстанавливаем приглашение
        std::cout << "> " << std::flush;
    }
}

void show_menu() {
    std::cout << "\n=================================" << std::endl;
    std::cout << "        MESSENGER CLIENT" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "  1. Login" << std::endl;
    std::cout << "  2. Register" << std::endl;
    std::cout << "  0. Exit" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Choose option: ";
}

bool connect_to_server() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        return false;
    }

    return true;
}

void disconnect_from_server() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

bool authenticate(const std::string& action, const std::string& login, const std::string& password) {
    if (!connect_to_server()) {
        std::cout << "[Error: Cannot connect to server]" << std::endl;
        return false;
    }

    std::string auth_cmd = action + " " + login + " " + password;
    send(sock, auth_cmd.c_str(), auth_cmd.size(), 0);

    char buffer[4096];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << buffer << std::endl;

        if (std::string(buffer).find("Error") != std::string::npos ||
            std::string(buffer).find("failed") != std::string::npos) {
            disconnect_from_server();
            return false;
        }
        return true;
    }

    disconnect_from_server();
    return false;
}

bool register_user(const std::string& login, const std::string& password, const std::string& birthday) {
    if (!connect_to_server()) {
        std::cout << "[Error: Cannot connect to server]" << std::endl;
        return false;
    }

    std::string auth_cmd = "REGISTER " + login + " " + password + " " + birthday;
    send(sock, auth_cmd.c_str(), auth_cmd.size(), 0);

    char buffer[4096];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << buffer << std::endl;

        if (std::string(buffer).find("Error") != std::string::npos ||
            std::string(buffer).find("already taken") != std::string::npos) {
            disconnect_from_server();
            return false;
        }
        return true;
    }

    disconnect_from_server();
    return false;
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    bool authenticated = false;

    while (!authenticated) {
        show_menu();

        int choice;
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 0) {
            std::cout << "Goodbye!" << std::endl;
#ifdef _WIN32
            WSACleanup();
#endif
            return 0;
        }
        else if (choice == 1) {
            std::string login, password;
            std::cout << "Login: ";
            std::getline(std::cin, login);
            std::cout << "Password: ";
            std::getline(std::cin, password);

            if (authenticate("LOGIN", login, password)) {
                authenticated = true;
            }
            else {
                std::cout << "\n[Login failed. Try again.]" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        else if (choice == 2) {
            std::string login, password, birthday;
            std::cout << "Login: ";
            std::getline(std::cin, login);
            std::cout << "Password: ";
            std::getline(std::cin, password);
            std::cout << "Birthday (YYYY-MM-DD): ";
            std::getline(std::cin, birthday);

            if (register_user(login, password, birthday)) {
                authenticated = true;
            }
            else {
                std::cout << "\n[Registration failed. Username may already exist.]" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        else {
            std::cout << "Invalid option. Please try again." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    // Запускаем поток для приёма сообщений
    std::thread receiver(receive_messages);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Основной цикл
    std::string input;
    while (connected) {
        //std::cout << "> " << std::flush;
        std::getline(std::cin, input);

        if (input.empty()) continue;

        // Команды
        if (input == "/exit") {
            if (in_chat) {
                // Выход из чата
                in_chat = false;
                std::cout << "[Exited chat]" << std::endl;
                send(sock, "/chats", 6, 0);
            }
            else {
                // Выход из программы
                send(sock, "/exit", 5, 0);
                connected = false;
                break;
            }
        }
        else if (input == "/chats" || input == "/list") {
            send(sock, input.c_str(), input.size(), 0);
        }
        else if (input.rfind("/msg ", 0) == 0) {
            // Поддерживаем /msg для обратной совместимости и создания новых чатов
            send(sock, input.c_str(), input.size(), 0);
        }
        else if (input.rfind("/open ", 0) == 0) {
            send(sock, input.c_str(), input.size(), 0);
            in_chat = true;
        }
        else if (input == "/help") {
            std::cout << "Commands: /chats, /open <chat_id>, /msg <user> <text>, /exit" << std::endl;
            std::cout << "In chat: just type your message" << std::endl;
        }
        else {
            // Пробуем интерпретировать как номер чата
            try {
                int chat_id = std::stoi(input);
                std::string cmd = "/open " + input;
                send(sock, cmd.c_str(), cmd.size(), 0);
                in_chat = true;
            }
            catch (...) {
                // Обычный текст — проверяем, в чате ли мы
                if (in_chat) {
                    send(sock, input.c_str(), input.size(), 0);
                }
                else {
                    std::cout << "Unknown command. Type /help" << std::endl << "> ";
                }
            }
        }
    }

    receiver.join();
    disconnect_from_server();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}