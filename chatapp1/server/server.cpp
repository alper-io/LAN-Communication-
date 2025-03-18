#define _WIN32_WINNT 0x501
#include <iostream>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

#define IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT "3504"
#define DEFAULT_BUFLEN 512
#define MAX_CLIENTS 100

struct client_type {
    int id;
    SOCKET socket = INVALID_SOCKET;
};

void client_handler(client_type& new_client, std::vector<client_type>& client_array) {
    char buffer[DEFAULT_BUFLEN];
    std::string msg;

    while (true) {
        memset(buffer, 0, DEFAULT_BUFLEN);

        int recv_result = recv(new_client.socket, buffer, DEFAULT_BUFLEN, 0);
        if (recv_result > 0) {
            buffer[recv_result] = '\0';
            msg = "Client #" + std::to_string(new_client.id) + ": " + buffer;
            std::cout << msg << std::endl;

            // Broadcast message to other clients
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (client_array[i].socket != INVALID_SOCKET && new_client.id != i) {
                    send(client_array[i].socket, msg.c_str(), msg.length(), 0);
                }
            }
        } else if (recv_result == 0) {
            msg = "Client #" + std::to_string(new_client.id) + " Disconnected";
            std::cout << msg << std::endl;
            break; // Disconnect on receive failure
        } else {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            break; // Disconnect on receive failure
        }
    }

    closesocket(new_client.socket);
    new_client.socket = INVALID_SOCKET;
}

int main() {
    WSADATA wsaData;
    struct addrinfo* result = nullptr, * ptr = nullptr, hints;
    SOCKET server_socket = INVALID_SOCKET;
    std::vector<client_type> clients(MAX_CLIENTS);

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
    hints.ai_flags = AI_PASSIVE;

    // Resolve server address and port
    if (getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed\n";
        WSACleanup();
        return 1;
    }

    // Create socket for server
    server_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Bind socket
    if (bind(server_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Listen on socket
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << DEFAULT_PORT << std::endl;

    while (true) {
        // Accept incoming connections
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            closesocket(server_socket);
            WSACleanup();
            return 1;
        }

        // Find available slot for client
        int client_id = -1;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].socket == INVALID_SOCKET) {
                clients[i].socket = client_socket;
                clients[i].id = i;
                client_id = i;
                break;
            }
        }

        if (client_id == -1) {
            std::cerr << "Too many clients connected\n";
            closesocket(client_socket);
            continue;
        }

        // Inform client of its ID
        std::string welcome_msg = "You are client #" + std::to_string(client_id) + "\n";
        send(client_socket, welcome_msg.c_str(), welcome_msg.length(), 0);
        std::cout << "Client #" << client_id << " connected\n";

        // Handle client communication in a new thread
        std::thread(client_handler, std::ref(clients[client_id]), std::ref(clients)).detach();
    }

    // Cleanup
    closesocket(server_socket);
    WSACleanup();

    return 0;
}