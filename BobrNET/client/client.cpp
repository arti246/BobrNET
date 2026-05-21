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

// Функция для приёма сообщений (работает в отдельном потоке)
void receive_messages() {
    char buffer[4096];
    while (connected) {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            std::cout << "\n[Disconnected from server]" << std::endl;
            connected = false;
            break;
        }
        buffer[bytes] = '\0';
        std::cout << buffer << std::flush;  // только вывод сообщения, без "> "
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed" << std::endl;
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    std::cout << "Connected to server!" << std::endl;

    // Аутентификация
    bool flag = true;
    std::string action, login, password;

    while (flag)
    {
        std::cout << "Enter REGISTER or LOGIN: ";
        std::getline(std::cin, action);

        if (action == "LOGIN" || action == "REGISTER")
        {
            flag = false;
        }
    }

    std::cout << "Login: ";
    std::getline(std::cin, login);

    std::cout << "Password: ";
    std::getline(std::cin, password);

    std::string auth_cmd = action + " " + login + " " + password;
    send(sock, auth_cmd.c_str(), auth_cmd.size(), 0);

    // Ждём ответ сервера на аутентификацию
    char buffer[4096];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << buffer << std::endl;
        if (std::string(buffer).find("Error") != std::string::npos ||
            std::string(buffer).find("failed") != std::string::npos) {
            closesocket(sock);
#ifdef _WIN32
            WSACleanup();
#endif
            return 1;
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

    // Ждём завершения потока приёма
    receiver.join();

    closesocket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}