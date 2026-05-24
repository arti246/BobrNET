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
bool need_prompt = true;
std::string current_user_name = "";   // имя текущего пользователя
std::string current_chat_name = "";   // имя собеседника в текущем чате

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

        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }

        if (msg.empty()) continue;

        // Парсим имя чата, если это заголовок
        if (msg.find("=== Chat:") == 0) {
            size_t start = 10;
            size_t end = msg.find(" ===", start);
            if (end != std::string::npos) {
                current_chat_name = msg.substr(start, end - start);
            }
        }

        // Определяем тип сообщения
        bool is_chat_message = (msg.find("] ") != std::string::npos &&
            msg[0] == '[' &&
            msg.find("===") == std::string::npos &&
            msg.find("=================") == std::string::npos);

        // Извлекаем имя отправителя из сообщения (формат: [время] Имя: текст)
        std::string sender_name = "";
        if (is_chat_message) {
            size_t bracket_end = msg.find("] ");
            if (bracket_end != std::string::npos && bracket_end + 2 < msg.length()) {
                size_t colon_pos = msg.find(":", bracket_end + 2);
                if (colon_pos != std::string::npos) {
                    sender_name = msg.substr(bracket_end + 2, colon_pos - (bracket_end + 2));
                }
            }
        }

        // Своё сообщение (отправитель — я)
        bool is_own_message = (!current_user_name.empty() && sender_name == current_user_name);

        // Сообщение от собеседника
        bool is_partner_message = (!current_chat_name.empty() && sender_name == current_chat_name);

        // Если в чате и сообщение не от меня и не от собеседника — игнорируем
        if (in_chat && is_chat_message && !is_own_message && !is_partner_message) {
            continue;
        }

        bool is_chat_history_header = (msg.find("=== Chat:") == 0);
        bool is_chat_history_separator = (msg.find("=========================") != std::string::npos);
        bool is_chat_list_header = (msg.find("=== Your chats ===") == 0);
        bool is_chat_list_separator = (msg.find("=================") != std::string::npos);

        bool is_system_notification = (msg.find("[Sent to") == 0) ||
            (msg.find("[Error") == 0) ||
            (msg.find("[Welcome") == 0) ||
            (msg.find("[No chats yet") == 0) ||
            (msg.find("Unknown command") == 0);

        bool is_offline_notification = (msg.find("[Delayed from") == 0);
        bool is_join_left_notification = (msg.find(" joined the chat") != std::string::npos) ||
            (msg.find(" left the chat") != std::string::npos);

        // Стираем текущую строку
        std::cout << "\r";
        std::cout << "\033[K";

        // Выводим сообщение
        std::cout << msg << std::endl;

        // Решаем, нужно ли показывать приглашение
        bool should_prompt = false;

        if (in_chat) {
            // В чате
            if (is_chat_history_separator) {
                should_prompt = true;
            }
            else if (is_own_message) {
                // Своё сообщение — после него показываем >
                should_prompt = true;
            }
            else if (!is_chat_message && !is_chat_history_header) {
                should_prompt = true;
            }
        }
        else {
            // Вне чата: показываем приглашение в большинстве случаев
            if (is_chat_list_separator) {
                should_prompt = true;
            }
            else if (is_system_notification) {
                should_prompt = true;
            }
            else if (is_offline_notification) {
                should_prompt = true;
            }
            else if (is_join_left_notification) {
                should_prompt = true;
            }
            else if (is_chat_message) {
                should_prompt = true;
            }
            else if (!is_chat_list_header && !is_chat_history_header && !is_chat_history_separator) {
                should_prompt = true;
            }
        }

        if (should_prompt) {
            std::cout << "> " << std::flush;
            need_prompt = false;
        }
        else {
            need_prompt = true;
        }
    }
}

void show_menu() {
    std::cout << "\n=================================" << std::endl;
    std::cout << "        BobrNET" << std::endl;
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
    inet_pton(AF_INET, "192.168.0.102", &serverAddr.sin_addr);

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

#ifdef _WIN32
#include <conio.h>

std::string get_password() {
    std::string password;
    char ch;
    std::cout << "Password: ";
    while (true) {
        ch = _getch();
        if (ch == '\r') {  // Enter
            std::cout << std::endl;
            break;
        }
        else if (ch == '\b') {  // Backspace
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
            }
        }
        else if (ch == 3) {  // Ctrl+C
            exit(0);
        }
        else {
            password.push_back(ch);
            std::cout << '*';
        }
    }
    return password;
}
#else
// Linux / Unix
#include <termios.h>
#include <unistd.h>

std::string get_password() {
    std::string password;
    termios oldt, newt;

    // Отключаем эхо (чтобы символы не отображались)
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cout << "Password: ";
    std::getline(std::cin, password);

    // Включаем эхо обратно
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;

    return password;
}
#endif

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    bool authenticated = false;
    std::string login, password;

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
            std::cout << "Login: ";
            std::getline(std::cin, login);
            password = get_password();

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
            password = get_password();
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

    current_user_name = login;

    // Запускаем поток для приёма сообщений
    std::thread receiver(receive_messages);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Основной цикл
    std::string input;
    while (connected) {
        if (need_prompt) {
            std::cout << "> " << std::flush;
            need_prompt = false;
        }

        std::getline(std::cin, input);

        if (input.empty()) {
            need_prompt = true;
            continue;
        }

        // Команды
        if (input == "/exit") {
            if (in_chat) {
                in_chat = false;
                current_chat_name = "";  // ← сброс
                std::cout << "[Exited chat]" << std::endl;
                send(sock, "/chats", 6, 0);
                need_prompt = true;
            }
            else {
                send(sock, "/exit", 5, 0);
                connected = false;
                break;
            }
        }
        else if (input == "/chats" || input == "/list") {
            send(sock, input.c_str(), input.size(), 0);
            need_prompt = true;
        }
        else if (input.rfind("/msg ", 0) == 0) {
            send(sock, input.c_str(), input.size(), 0);
            need_prompt = true;
        }
        else if (input.rfind("/open ", 0) == 0) {
            send(sock, input.c_str(), input.size(), 0);
            in_chat = true;
            need_prompt = true;
        }
        else if (input == "/help") {
            std::cout << "Commands: /chats, /open <chat_id>, /msg <user> <text>, /exit" << std::endl;
            std::cout << "In chat: just type your message" << std::endl;
            need_prompt = true;
        }
        else {
            try {
                int chat_id = std::stoi(input);
                std::string cmd = "/open " + input;
                send(sock, cmd.c_str(), cmd.size(), 0);
                in_chat = true;
                need_prompt = true;
            }
            catch (...) {
                if (in_chat) {
                    send(sock, input.c_str(), input.size(), 0);
                    need_prompt = true;
                }
                else {
                    std::cout << "Unknown command. Type /help" << std::endl;
                    need_prompt = true;
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