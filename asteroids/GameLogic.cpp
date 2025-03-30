#include "GameLogic.h"
#include "Asteroid.h"
#include "Player.h"
#include "Bullet.h"

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#pragma comment(lib, "ws2_32.lib")  // Link Winsock library

#define SERVER_IP "192.168.1.15"  // Replace with your server's IP
#define SERVER_PORT 3001
SOCKET udpSocket;
sockaddr_in serverAddr;
bool isRunning = true;  // Used for network thread

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

// Initialize UDP connection
void initNetwork() {
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
    ioctlsocket(udpSocket, FIONBIO, &mode);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // Send connection request
    char connRequest = CONN_REQUEST;
    int sendResult = sendto(udpSocket, &connRequest, sizeof(connRequest), 0,
        (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "Failed to send connection request. Error: " << WSAGetLastError() << "\n";
    }
    else {
        std::cout << "Sent connection request to server.\n";
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

// Separate thread to handle incoming game state

void receiveGameState() {
    while (isRunning) {
        char buffer[512];
        sockaddr_in fromAddr;
        int fromSize = sizeof(fromAddr);

        int recvLen = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
            (sockaddr*)&fromAddr, &fromSize);
        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            std::cout << "Game State Received: " << buffer << "\n";
            // Parse the packet based on your protocol (e.g., check the first byte for the command type)
            if (buffer[0] == START_GAME) {
                // Start the game logic
                GameLogic::start();
            }
        }
        else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::cerr << "Recv failed. Error: " << error << "\n";
            }
        }
        Sleep(10);
    }
}
// Cleanup
void closeNetwork() {
    isRunning = false;
    closesocket(udpSocket);
    WSACleanup();
    std::cout << "Network closed.\n";
}

void startNetworkThread() {
    std::thread recvThread(receiveGameState);
    recvThread.detach();
    std::cout << "Started network thread.\n";
}


// Entities lists
std::vector<Entity*> GameLogic::entities{};
std::list<Entity*> GameLogic::entitiesToDelete{};
std::list<Entity*> GameLogic::entitiesToAdd{};
std::vector<Player*> GameLogic::players{};

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
    is_game_over = false;
    asteroid_spawn_time = ASTEROID_SPAWN_TIME;
    players.clear();

    std::vector<sf::Color> player_colors = {
        sf::Color::White,
        sf::Color::Blue,
        sf::Color::Green,
        sf::Color::Yellow
    };

    // Add players with different colors
    for (size_t i = 0; i < player_colors.size(); i++) {
        Player* new_player = new Player(i+1, player_colors[i]);
        players.push_back(new_player);
        entitiesToAdd.push_back(new_player);
    }

    entitiesToAdd.push_back(new Asteroid());
    game_timer = 60.f; // Game lasts for 60 seconds (adjust as needed)

}

void GameLogic::update(sf::RenderWindow& window, float delta_time) {
    if (!is_game_over) {
        game_timer -= delta_time;
        if (game_timer <= 0.f) {
            gameOver(); 
            entities.clear();
        }
    }

    window.clear();
    
    // Test commands
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) sendPlayerInput(CONN_REQUEST);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) sendPlayerInput(ACK_CONN_REQUEST);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) sendPlayerInput(REQ_START_GAME);
    //if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4)) sendPlayerInput('D');

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

    int i = 0;
    for (Player* player : players) {
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
            entities.clear();
            start();
        }

       
    }

}

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