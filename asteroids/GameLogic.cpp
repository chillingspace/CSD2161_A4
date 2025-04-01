#include "GameLogic.h"
#include "Asteroid.h"
#include "Player.h"
#include "Bullet.h"

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")  // Link Winsock library

#define JS_DEBUG

#ifndef JS_DEBUG

std::string SERVER_IP;
int SERVER_PORT;
#else
#define SERVER_IP "192.168.1.15"  // Replace with your server's IP
#define SERVER_PORT 3000
#endif

SOCKET udpSocket;
sockaddr_in serverAddr;

uint16_t udpBroadcastPort; 
uint16_t udpClientBroadcastPort;

bool isRunning = true;  // Used for network thread

static constexpr int MAX_PACKET_SIZE = 1000;

enum CLIENT_REQUESTS {
    CONN_REQUEST = 0,
    ACK_CONN_REQUEST,
    REQ_START_GAME,
    ACK_START_GAME,
    ACK_ACK_SELF_SPACESHIP,
    SELF_SPACESHIP,
    ACK_ALL_ENTITIES,
    ACK_END_GAME
};

enum SERVER_MSGS {
    CONN_ACCEPTED = 0,
    CONN_REJECTED,
    START_GAME,
    ACK_SELF_SPACESHIP,
    ALL_ENTITIES,
    END_GAME
};

#pragma pack(push, 1) // Ensure no padding in struct
struct PlayerMovementPacket {
    uint8_t cmd;
    uint8_t sessionID;
    float posX;
    float posY;
    float vecX;
    float vecY;
    float rotation;
};
#pragma pack(pop)

// Entities lists
std::vector<Entity*> GameLogic::entities{};
std::list<Entity*> GameLogic::entitiesToDelete{};
std::list<Entity*> GameLogic::entitiesToAdd{};
std::unordered_map<uint8_t, Player*> GameLogic::players{};

// Colors of the player
std::vector<sf::Color> player_colors = {
    sf::Color::Magenta,
    sf::Color::Blue,
    sf::Color::Green,
    sf::Color::Yellow
};

void listenForBroadcast() {
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "Error creating UDP socket: " << WSAGetLastError() << std::endl;
        return;
    }

    // Allow broadcast
    BOOL broadcastEnable = TRUE;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable)) == SOCKET_ERROR) {
        std::cerr << "Failed to set broadcast option: " << WSAGetLastError() << std::endl;
        closesocket(udpSocket);
        return;
    }

    // Bind socket to the broadcast port (udpBroadcastPort)
    sockaddr_in recvAddr;
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_addr.s_addr = INADDR_ANY;  // Listen on all network interfaces
    recvAddr.sin_port = htons(udpBroadcastPort);

    if (bind(udpSocket, (sockaddr*)&recvAddr, sizeof(recvAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(udpSocket);
        return;
    }

    std::cout << "Listening for UDP broadcasts on port " << udpBroadcastPort << "..." << std::endl;

    while (true) {
        char buffer[1024];
        sockaddr_in senderAddr;
        int senderAddrSize = sizeof(senderAddr);

        int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&senderAddr, &senderAddrSize);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Received broadcast from " << SERVER_IP << ": " << buffer << std::endl;
        }
    }

    closesocket(udpSocket);
    WSACleanup();
}


// Initialize UDP connection
void initNetwork() {
    
#ifndef JS_DEBUG
    std::cout << "Enter Server IP Address: ";
    std::getline(std::cin, SERVER_IP);

    std::string portInput;
    std::cout << "Enter Server Port: ";
    std::getline(std::cin, portInput);

    try {
        SERVER_PORT = std::stoi(portInput);
    }
    catch (const std::exception& e) {
        std::cerr << "Invalid port number. Using default port 1111.\n";
        SERVER_PORT = 1111;
    }
#endif
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        exit(1);
    }

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        exit(1);
    }
    std::cout << "UDP socket created successfully.\n";

    u_long mode = 1;
    ioctlsocket(udpSocket, FIONBIO, &mode);  // Set to non-blocking mode

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
#ifndef JS_DEBUG
    inet_pton(AF_INET, SERVER_IP.c_str(), &serverAddr.sin_addr);
#else
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
#endif
    // Send connection request
    char connRequest = CONN_REQUEST;
    int sendResult = sendto(udpSocket, &connRequest, sizeof(connRequest), 0,
        (sockaddr*)&serverAddr, sizeof(serverAddr));

    if (sendResult == SOCKET_ERROR) {
        std::cerr << "Failed to send connection request. Error: " << WSAGetLastError() << "\n";
        return;
    }

    std::cout << "Sent connection request to server. Waiting for response...\n";

    // Wait for server response
    char buffer[512];
    sockaddr_in fromAddr;
    int fromSize = sizeof(fromAddr);
    int retries = 10;  // Wait for a few tries
    bool connected = false;

    while (retries-- > 0) {
        int recvLen = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (sockaddr*)&fromAddr, &fromSize);

        if (recvLen > 0) {
            uint8_t serverMsg = buffer[0];
            std::cout << "Receiving from server!" << serverMsg << std::endl;

            if (serverMsg == CONN_ACCEPTED) {
                std::cout << "Connection accepted by server!\n";
                connected = true;

                int offset = 1; // Start after the command byte

                // Session ID (1 byte)
                uint8_t sessionID = buffer[offset++];

                // Read UDP Broadcast Port (2 bytes, ensure correct endian conversion)
                memcpy(&udpBroadcastPort, buffer + offset, sizeof(uint16_t));
                udpBroadcastPort = ntohs(udpBroadcastPort); // Convert from network byte order if needed
                offset += 2;
                // Create and bind the second UDP socket for the broadcast port
                udpClientBroadcastPort = udpBroadcastPort - 2;

                std::cout << udpClientBroadcastPort << std::endl;

                // Read Spawn X (4 bytes, float)
                float spawnPosX;
                spawnPosX = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                offset += sizeof(float);

                // Read Spawn Y (4 bytes, float)
                float spawnPosY;
                spawnPosY = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                offset += sizeof(float);

                // Read Spawn Rotation (4 bytes, float)
                float spawnRotation;
                spawnRotation = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                offset += sizeof(float);

                std::cout << "Session ID: " << (int)sessionID << std::endl;
                std::cout << "UDP Broadcast Port: " << udpBroadcastPort << std::endl;
                std::cout << "Spawn X: " << spawnPosX << std::endl;
                std::cout << "Spawn Y: " << spawnPosY << std::endl;
                std::cout << "Spawn Rotation: " << spawnRotation << " degrees" << std::endl;

                Player* new_player = new Player(sessionID, player_colors[sessionID], sf::Vector2f(spawnPosX, spawnPosY), spawnRotation);
                GameLogic::players[sessionID] = new_player; // Store in map
                GameLogic::entitiesToAdd.push_back(new_player);

                // Send ACK_CONN_REQUEST
                char ack = ACK_CONN_REQUEST;
                sendResult = sendto(udpSocket, &ack, sizeof(ack), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

                if (sendResult == SOCKET_ERROR) {
                    std::cerr << "Failed to send ACK_CONN_REQUEST. Error: " << WSAGetLastError() << "\n";
                }
                else {
                    std::cout << "Sent ACK_CONN_REQUEST to server.\n";
                }

                break;
            }
            else if (serverMsg == CONN_REJECTED) {
                std::cerr << "Connection rejected by server.\n";
                return;
            }
        }

        Sleep(500);  // Wait before retrying
    }

}

// Send player input
void sendPlayerInput(char input) {
    int sendResult = sendto(udpSocket, &input, sizeof(input), 0,
        (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "Failed to send player input. Error: " << WSAGetLastError() << "\n";
    }
    else {
        std::cout << "Sending data" << std::endl;
    }
}

//
//void sendPlayerMovement(uint8_t sessionID, float posX, float posY, float vecX, float vecY, float rotation) {
//    PlayerMovementPacket packet;
//    packet.cmd = 1; // Movement command
//    packet.sessionID = sessionID;
//    packet.posX = posX;
//    packet.posY = posY;
//    packet.vecX = vecX;
//    packet.vecY = vecY;
//    packet.rotation = rotation;
//
//    int sendResult = sendto(udpSocket, (char*)&packet, sizeof(packet), 0,
//        (sockaddr*)&serverAddr, sizeof(serverAddr));
//    if (sendResult == SOCKET_ERROR) {
//        std::cerr << "Failed to send player movement. Error: " << WSAGetLastError() << "\n";
//    }
//}

// Separate thread to handle incoming game state

void startNetworkThread() {
    std::thread recvThread(listenForBroadcast);
    recvThread.detach();
    std::cout << "Started network thread.\n";

    //// Start broadcast listener in a separate thread
    //if (broadcastSocket == INVALID_SOCKET) {
    //    std::cerr << "Broadcast socket creation failed. Aborting network thread." << std::endl;
    //    return;
    //}
    //std::thread broadcastThread(receiveBroadcastData, broadcastSocket);
    //broadcastThread.detach();
    //std::cout << "Started broadcast listener thread.\n";
}


// Cleanup
void closeNetwork() {
    isRunning = false;
    closesocket(udpSocket);
    WSACleanup();
    std::cout << "Network closed.\n";
}

//
//void receivePlayerMovement(char* buffer, int size) {
//    if (size < sizeof(PlayerMovementPacket)) return;
//
//    PlayerMovementPacket* packet = (PlayerMovementPacket*)buffer;
//
//    // Find the player with this session ID
//    Player* player = findPlayerBySessionID(packet->sessionID);
//    if (player) {
//        player->position.x = packet->posX;
//        player->position.y = packet->posY;
//        player->velocity.x = packet->vecX;
//        player->velocity.y = packet->vecY;
//        player->rotation = packet->rotation;
//    }
//}

// Game conditions
float GameLogic::game_timer;
bool GameLogic::is_game_over;

// Asteroids conditions
float GameLogic::asteroid_spawn_time;


// Font and text
sf::Font font;
sf::Text game_over_text;
sf::Text player_score_text;



void GameLogic::init() {
    initNetwork();
    startNetworkThread();  // Start receiving data

    // Score Init
    font.loadFromFile("Arial Italic.ttf");

    player_score_text.setFont(font);
    player_score_text.setCharacterSize(30);

    game_over_text.setFont(font);
    game_over_text.setFillColor(sf::Color::White);
    game_over_text.setPosition(sf::Vector2f(200.f, SCREEN_HEIGHT / 2.f)); // Adjust as needed
    game_over_text.setCharacterSize(60);

    is_game_over = true;
}

void GameLogic::start() {
    // TO BE MOVED TO SERVER
    {
        is_game_over = false;
        asteroid_spawn_time = ASTEROID_SPAWN_TIME;
        //players.clear();
    }


    // TO BE MOVED TO SERVER (COLOR DOES ISNT REQUIRED)
    {


        entitiesToAdd.push_back(new Asteroid());
        game_timer = 60.f; // Game lasts for 60 seconds (adjust as needed)
    }
}

void GameLogic::update(sf::RenderWindow& window, float delta_time) {

    // TO BE MOVED TO SERVER
    if (!is_game_over) {
        game_timer -= delta_time;
        if (game_timer <= 0.f) {
            gameOver(); 
            entities.clear();
        }
    }

    window.clear();
    
    // Test commands
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) sendPlayerInput(REQ_START_GAME);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) sendPlayerInput(ACK_CONN_REQUEST);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3))  sendPlayerInput(ACK_CONN_REQUEST);
    //if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) sendPlayerInput('D');

    // TO BE MOVED TO SERVER
    {
        asteroid_spawn_time -= delta_time;

        for (auto& entity : entitiesToAdd) {
            entities.push_back(entity);
        }
        entitiesToAdd.clear();

        for (size_t i = 0; i < entities.size(); i++) {
            entities[i]->update(delta_time);
            entities[i]->render(window);
        }

        for (auto* entity : entitiesToDelete) {
            auto it = std::find(entities.begin(), entities.end(), entity);
            if (it != entities.end()) {
                delete* it;       // Delete the object
                entities.erase(it); // Remove from the list
            }
        }
        entitiesToDelete.clear();

        if (asteroid_spawn_time <= 0.f && entities.size() <= 12) {
            entities.push_back(new Asteroid());
            entities.push_back(new Asteroid());
            asteroid_spawn_time = ASTEROID_SPAWN_TIME;
        }
    }

    // Draw scoreboard based on num of players
    int i = 0;
    for (auto& [sessionID, player] : players){
        player_score_text.setPosition(sf::Vector2f(40.f, 20.f + (i * 80.f)));
        player_score_text.setString("P" + std::to_string(i + 1) + "\nScore: " + std::to_string(player->score));
        player_score_text.setFillColor(player->player_color);
        window.draw(player_score_text);
        i++;
    }

    // Show timer on screen
    sf::Text timer_text;
    timer_text.setFont(font);
    timer_text.setCharacterSize(30);
    timer_text.setPosition(sf::Vector2f(SCREEN_WIDTH / 2 - 40.f, 20.f)); // Adjust position as needed
    timer_text.setString("Time: " + std::to_string(static_cast<int>(game_timer))); // Convert to int for display


    window.draw(timer_text);


    if (is_game_over) {
        game_over_text.setString("Press Enter to Start a New Match");
        window.draw(game_over_text);

        // Wait for Enter key to restart
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
            //entities.clear();
            start();
        }

       
    }

}

// TO BE MOVED TO SERVER
Player* GameLogic::findPlayerBySession(uint8_t sessionID)
{
    auto it = players.find(sessionID);
    return (it != players.end()) ? it->second : nullptr;
}

// TO BE MOVED TO SERVER
bool GameLogic::checkCollision(Entity* a, Entity* b) {
    // Compute the distance between the two entities
    float distance = sqrt(pow(a->position.x - b->position.x, 2) +
        pow(a->position.y - b->position.y, 2));

    // Determine radius (approximate using bounding boxes)
    float a_radius = 0.f, b_radius = 0.f;

    if (Asteroid* asteroid = dynamic_cast<Asteroid*>(a)) {
        a_radius = std::max(asteroid->shape.getLocalBounds().width,
            asteroid->shape.getLocalBounds().height) / 2.f;
    }
    else if (Player* player = dynamic_cast<Player*>(a)) {
        a_radius = 20.f; // Approximate player size (adjust based on your ship size)
    }
    else if (Bullet* bullet = dynamic_cast<Bullet*>(a)) {
        a_radius = 5.f;  // Small bullet size
    }

    if (Asteroid* asteroid = dynamic_cast<Asteroid*>(b)) {
        b_radius = std::max(asteroid->shape.getLocalBounds().width,
            asteroid->shape.getLocalBounds().height) / 2.f;
    }
    else if (Player* player = dynamic_cast<Player*>(b)) {
        b_radius = 20.f; // Approximate player size
    }
    else if (Bullet* bullet = dynamic_cast<Bullet*>(b)) {
        b_radius = 5.f;  // Small bullet size
    }

    // Collision check (distance must be smaller than sum of radii)
    return distance < (a_radius + b_radius);
}

void GameLogic::gameOver() {
    is_game_over = true;

}