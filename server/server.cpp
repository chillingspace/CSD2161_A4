#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 54000
#define BUFFER_SIZE 1024

struct Player {
    sockaddr_in address;
    int x, y;
};

// Store connected clients and their game state
std::unordered_map<std::string, Player> players;

void ProcessClientInput(const std::string& clientId, const std::string& input) {
    // Simple movement handling
    if (input == "MOVE_LEFT") players[clientId].x -= 5;
    else if (input == "MOVE_RIGHT") players[clientId].x += 5;
    else if (input == "MOVE_UP") players[clientId].y -= 5;
    else if (input == "MOVE_DOWN") players[clientId].y += 5;
}

std::string SerializeGameState() {
    std::string gameState;
    for (const auto& [id, player] : players) {
        gameState += id + " " + std::to_string(player.x) + " " + std::to_string(player.y) + "\n";
    }
    return gameState;
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE];

    // Initialize Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Create a UDP socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        return -1;
    }

    // Bind server to a port
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    std::cout << "Server started on port " << SERVER_PORT << "\n";

    while (true) {
        int clientLen = sizeof(clientAddr);
        int bytesReceived = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, (sockaddr*)&clientAddr, &clientLen);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            char clientIp[INET_ADDRSTRLEN]; // Buffer for IP address
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
            std::string clientIpStr(clientIp); // Convert the char array to std::string


            std::string clientMsg(buffer);

            // Register client if not already in list
            if (players.find(clientIpStr) == players.end()) {
                players[clientIpStr] = { clientAddr, 400, 300 }; // Default start position
                std::cout << "New player connected: " << clientIpStr << "\n";
            }

            // Process input and update state
            ProcessClientInput(clientIpStr, clientMsg);

            // Send updated game state to all clients
            std::string gameState = SerializeGameState();
            for (const auto& [id, player] : players) {
                sendto(serverSocket, gameState.c_str(), gameState.size(), 0,
                    (sockaddr*)&player.address, sizeof(player.address));
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
