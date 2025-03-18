#define _WIN32_WINNT 0x501
#include <iostream>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

#define IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT "3504"
#define DEFAULT_BUFLEN 512

void receive_messages(SOCKET client_socket) {
    char buffer[DEFAULT_BUFLEN];
    while (true) {
        memset(buffer, 0, DEFAULT_BUFLEN);
        int recv_result = recv(client_socket, buffer, DEFAULT_BUFLEN, 0);
        if (recv_result > 0) {
            buffer[recv_result] = '\0';
            std::cout << buffer << std::endl;
        } else if (recv_result == 0) {
            std::cout << "Connection closed by server\n";
            break;
        } else {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET client_socket = INVALID_SOCKET;
    struct addrinfo* result = nullptr, * ptr = nullptr, hints;
    std::string msg;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // Setup hints
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve server address and port
    if (getaddrinfo(IP_ADDRESS, DEFAULT_PORT, &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed\n";
        WSACleanup();
        return 1;
    }

    // Create socket for connecting to server
    client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Connect to server
    if (connect(client_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    std::cout << "Connected to server\n";

    // Start thread to receive messages
    std::thread recv_thread(receive_messages, client_socket);
    recv_thread.detach();

    // Send messages
    while (true) {
        std::getline(std::cin, msg);
        if (send(client_socket, msg.c_str(), msg.length(), 0) == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Cleanup
    closesocket(client_socket);
    WSACleanup();

    return 0;
}