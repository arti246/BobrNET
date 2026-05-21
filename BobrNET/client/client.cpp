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

void receive_messages() {
    char buffer[4096];
    while (connected) {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            connected = false;
            break;
        }
        buffer[bytes] = '\0';
        std::cout << buffer << std::flush;
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
    // Подключаемся к серверу
    if (!connect_to_server()) {
        std::cout << "[Error: Cannot connect to server. Make sure it's running.]" << std::endl;
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
            std::string(buffer).find("failed") != std::string::npos ||
            std::string(buffer).find("already taken") != std::string::npos) {
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
        std::cout << "[Error: Cannot connect to server. Make sure it's running.]" << std::endl;
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

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Основной цикл для отправки сообщений
    std::string input;
    while (connected) {
        std::cout << "> " << std::flush;
        std::getline(std::cin, input);

        if (input == "/exit") {
            send(sock, input.c_str(), input.size(), 0);
            connected = false;
            break;
        }

        send(sock, input.c_str(), input.size(), 0);
    }

    receiver.join();
    disconnect_from_server();
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}