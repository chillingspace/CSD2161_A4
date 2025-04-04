#include "GameLogic.h"
#include "Asteroid.h"
#include "Player.h"
#include "Bullet.h"

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <chrono>
#include <unordered_set>
#pragma comment(lib, "ws2_32.lib")  // Link Winsock library

/*#define LOCALHOST_DEV*/  // for developing on 1 machine

std::string playername;
//
//#define JS_DEBUG

#ifndef JS_DEBUG

std::string SERVER_IP;
int SERVER_PORT;
#else
//#define SERVER_IP "192.168.1.15"  // Replace with your server's IP
#define SERVER_IP "169.254.253.95"
#define SERVER_PORT 3001
#endif

std::thread recvUdpThread;
std::thread keepAliveThread;

SOCKET udpSocket;
sockaddr_in serverAddr;

uint8_t current_session_id;

bool isRunning = true;  // Used for network thread

int seq{};      // for packets that require ack
std::unordered_set<int> acked_seq;      // acked sequence numbers, once processed, will be popped
std::mutex acked_seq_mutex;

static constexpr int MAX_PACKET_SIZE = 1000;

enum CLIENT_REQUESTS {
    CONN_REQUEST = 0,
    ACK_CONN_REQUEST,
    REQ_START_GAME,
    ACK_START_GAME,
    SELF_SPACESHIP,
    NEW_BULLET,
    ACK_END_GAME,
    KEEP_ALIVE
};

enum SERVER_MSGS {
    CONN_ACCEPTED = 0,
    CONN_REJECTED,
    START_GAME,
    ACK_NEW_BULLET,
    ALL_ENTITIES,
    END_GAME,
};

// Entities lists
std::vector<Entity*> GameLogic::entities{};
std::unordered_map<uint8_t, Player*> GameLogic::players{};
std::unordered_map<int, std::string> playersNames;
std::map<int, std::string, std::greater<int>> leaderboard;

// Store the most recent entity updates in a global variable
std::vector<Player> updatedPlayers;
std::vector<Bullet> updatedBullets;
std::vector<Asteroid> updatedAsteroids;
bool updatedEntities;

// Game conditions
int GameLogic::game_timer;
bool GameLogic::is_game_over;
uint8_t winner_id;
uint8_t winner_score;

// Asteroids conditions
float GameLogic::asteroid_spawn_time;

// Font and text
sf::Font font;
sf::Text game_over_text;
sf::Text player_score_text;
sf::Text player_leaderboard;

// Colors of the player
std::vector<sf::Color> player_colors = {
    sf::Color::Magenta,
    sf::Color::Blue,
    sf::Color::Green,
    sf::Color::Yellow
};

// Send player input
void sendData(const std::vector<char>& buffer) {
    int sendResult = sendto(udpSocket, buffer.data(), buffer.size(), 0,
        (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "Failed to send data: " << WSAGetLastError() << "\n";
    }
    else {
        //std::cout << "Sending data" << std::endl;
    }
}

// Handles ack from server
void listenForUdpMessages() {
    char buffer[MAX_PACKET_SIZE];
    sockaddr_in senderAddr;
    int senderAddrSize = sizeof(senderAddr);

    std::cout << "Listening for UDP messages...\n";

    while (isRunning) {
        int bytesReceived = recvfrom(udpSocket, buffer, MAX_PACKET_SIZE, 0,
            (sockaddr*)&senderAddr, &senderAddrSize);

        if (bytesReceived > 0) {
            uint8_t cmd = buffer[0]; // First byte is the command identifier
            std::vector<char> send_buffer;
            switch (cmd) {
            case CONN_ACCEPTED:
            {
                send_buffer.clear();
                send_buffer.push_back(ACK_CONN_REQUEST);
                send_buffer.push_back(current_session_id);

                sendData(send_buffer);
                break;
            }
            case ACK_NEW_BULLET:
            {
                std::lock_guard<std::mutex> aclock(acked_seq_mutex);
                acked_seq.insert(buffer[1] << 24 | buffer[2] << 16 | buffer[3] << 8 | buffer[4]);
            }

             break;
            case START_GAME:
            {

                if (GameLogic::is_game_over) {
                    GameLogic::start();
                    std::cout << "starting" << std::endl;
                }


                int offset = 2;
                for (int i{}; i < buffer[1]; i++) {     // iterate through num players
                    int sid = buffer[offset++];
                    int playernamesize = buffer[offset++];
                    std::string playerName;
                    for (int j{}; j < playernamesize; j++) {    // iterate through num chars in player name
                        playerName += buffer[offset + j];
                    }

                    playersNames[sid] = playerName;
                    offset += playernamesize;

                    // Check if player already exists
                    if (GameLogic::players.count(sid)) {
                        std::cout << "Player with SID " << sid << " already exists." << std::endl;
                        continue;  // Skip adding duplicate player
                    }

                    Player* new_player = new Player(sid, player_colors[sid],
                        sf::Vector2f(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f), 0.f);
                    GameLogic::players[sid] = new_player;  // Store in map
                    GameLogic::entities.push_back(new_player);
                }

                // Example: Send ACK_START_GAME back to sender
                send_buffer = { ACK_START_GAME };
                sendData(send_buffer);
            }
            break;
            case ALL_ENTITIES: {
                //std::cout << "Received ALL_ENTITIES update (" << bytesReceived << " bytes).\n";

                if (bytesReceived < 2) {
                    std::cerr << "Error: Packet too short for ALL_ENTITIES\n";
                    break;
                }

                int offset = 1;  // Skip packet type
                GameLogic::game_timer = buffer[offset++];
                int num_spaceships = buffer[offset++];
                updatedPlayers.clear();

                // Read Player Data (15 bytes per spaceship)
                for (int i = 0; i < num_spaceships; i++) {
                    if (offset + 15 > bytesReceived) {
                        std::cerr << "Error: Not enough bytes for spaceship data\n";
                        break;
                    }

                    Player s;
                    s.sid = buffer[offset++];
                    s.position.x = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                    offset += 4;
                    s.position.y = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                    offset += 4;
                    s.angle = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                    offset += 4;
                    s.lives_left = buffer[offset++];  // 1 byte for lives_left (no need for memcpy)

                    s.score = buffer[offset++];  // 1 byte for score (no need for memcpy)
                    s.player_color = player_colors[s.sid];

                    if (s.lives_left == 0) {
                        continue;
                    }

                    updatedPlayers.push_back(s);
                    updatedEntities = true;
                    //std::cout << "Spaceship SID: " << static_cast<int>(s.sid)
                    //    << " | Pos: (" << s.position.x << ", " << s.position.y << ")"
                    //    << " | Angle : " << s.angle
                    //    << " | Lives: " << (int)s.lives_left
                    //    << " | Score: " << (int)s.score << "\n";
                }

                int num_bullets = buffer[offset++];

                updatedBullets.clear();
                // Read Bullet Data (9 bytes per bullet)
                for (int i = 0; i < num_bullets; i++) {
                    if (offset + 9 > bytesReceived) {
                        std::cerr << "Error: Not enough bytes for bullet data\n";
                        break;
                    }

                    Bullet b;
                    b.sid = buffer[offset++];
                    b.position.x = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                    offset += 4;
                    b.position.y = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                    offset += 4;
                    b.setColor(player_colors[b.sid]);
                    updatedBullets.push_back(b);
                    updatedEntities = true;
                    //std::cout << "Bullet Owner SID: " << b.sid
                    //    << " | Pos: (" << b.position.x << ", " << b.position.y << ")\n";
                }

                int num_asteroids = buffer[offset++];

                updatedAsteroids.clear();

                // Read Asteroid Data (8 bytes per asteroid)
                for (int i = 0; i < num_asteroids; i++) {
                    if (offset + 8 > bytesReceived) {
                        std::cerr << "Error: Not enough bytes for asteroid data\n";
                        break;
                    }

                    Asteroid a;

                    a.position.x = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                    offset += sizeof(float);
                    a.position.y = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                    offset += sizeof(float);
                    a.radius = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
                    offset += sizeof(float);

                    updatedAsteroids.push_back(a);
                    updatedEntities = true;
                    //std::cout << "Asteroid Pos: (" << a.position.x << ", " << a.position.y << ") Radius = " << a.radius << "\n";
                }


                break;
            }

            case END_GAME:

                int offset = 1;
                winner_id = buffer[offset++];
                winner_score = buffer[offset++];
                uint8_t num_high_scores = buffer[offset++];

                for (int i = 0; i < num_high_scores; i++) {
                    int score = buffer[offset++];
                    int name_length = buffer[offset++];

                    std::string name;
                    for (int j = 0; j < name_length; ++j) {
                        name.push_back(buffer[offset + j]);
                    }
                    offset += name_length;

                    // Add to the leaderboard
                    Global::addToLeaderboard(leaderboard, score, name);
                }

                GameLogic::gameOver();
                // Send ACK
                send_buffer = { ACK_END_GAME };
                sendData(send_buffer);
                break;

            }
        }
        else if (bytesReceived == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
            std::cerr << "UDP recvfrom error: " << WSAGetLastError() << "\n";
        }

        Sleep(1);  // Prevent high CPU usage
    }

    //closesocket(udpSocket);
}


// Initialize UDP connection
bool initNetwork() {
    
#ifndef JS_DEBUG
    std::cout << "Enter Server IP Address: ";
    std::getline(std::cin, SERVER_IP);

    std::string portInput;
    std::cout << "Enter Server Port: ";
    std::getline(std::cin, portInput);

    std::cout << "Name: ";
    std::getline(std::cin, playername);

    try {
        SERVER_PORT = std::stoi(portInput);
    }
    catch (const std::exception& e) {
        std::cerr << "Invalid port number. Using default port 1111.\n";
        SERVER_PORT = 1111;
    }
#else
    playername = "GWEE BOON XUEN SEAN";
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

    // HANDLE 
    // 
    //  HANDSHAKE FIRST
    
    // Send connection request
    std::vector<char> conn_buffer = { CONN_REQUEST };
    conn_buffer.push_back((char)playername.size());
    conn_buffer.insert(conn_buffer.end(), playername.begin(), playername.end());
    sendData(conn_buffer);
    std::cout << "Sent connection request to server. Waiting for response...\n";

    // Wait for server response
    char buffer[512];
    sockaddr_in fromAddr;
    int fromSize = sizeof(fromAddr);
    bool connected = false;

    auto start = std::chrono::high_resolution_clock::now();
    constexpr float timeoutMs = 10000.f;

    while (true) {
        auto curr = std::chrono::high_resolution_clock::now();
        float elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(curr - start).count();
        if (elapsedMs > timeoutMs) {
            std::cout << "Connection request timed out." << std::endl;
            break;
        }

        int recvLen = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (sockaddr*)&fromAddr, &fromSize);

        if (recvLen > 0) {
            uint8_t serverMsg = buffer[0];
            std::cout << "Receiving from server!" << serverMsg << std::endl;

            if (serverMsg == CONN_ACCEPTED) {
                std::cout << "Connection accepted by server!\n";
                connected = true;

                int offset = 1; // Start after the command byte

                // Session ID (1 byte)
                current_session_id = buffer[offset++];

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

                std::cout << "Session ID: " << (int)current_session_id << std::endl;
                std::cout << "Spawn X: " << spawnPosX << std::endl;
                std::cout << "Spawn Y: " << spawnPosY << std::endl;
                std::cout << "Spawn Rotation: " << spawnRotation << " degrees" << std::endl;

                Player* current_player = new Player(current_session_id, player_colors[current_session_id], sf::Vector2f(spawnPosX, spawnPosY), spawnRotation);
                GameLogic::players[current_session_id] = current_player; // Store in map
                GameLogic::entities.push_back(current_player);

                // Send ACK_CONN_REQUEST
                conn_buffer.clear();
                conn_buffer.push_back(ACK_CONN_REQUEST);
                sendData(conn_buffer);
                break;
            }
            else if (serverMsg == CONN_REJECTED) {
                std::cerr << "Connection rejected by server.\n";
                return false;
            }
        }
    }

    if (!connected)
        std::cerr << "Failed to connect to server" << std::endl;

    return connected;
}

// Separate thread to handle incoming game state
void startNetworkThread() {
    recvUdpThread = std::thread(listenForUdpMessages);
    //recvBroadcastThread = std::thread(listenForBroadcast);
    keepAliveThread = std::thread([]() {
        std::vector<char> buf;
        buf.push_back(KEEP_ALIVE);
        buf.push_back(current_session_id);
        while (isRunning) {
            sendData(buf);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        }
    );
    std::cout << "Started network threads.\n";
}


// Cleanup
void closeNetwork() {
    isRunning = false; // Signal threads to stop

    if (recvUdpThread.joinable()) {
        recvUdpThread.join();
    }

    //if (recvBroadcastThread.joinable()) {
    //    recvBroadcastThread.join();
    //}

    if (keepAliveThread.joinable()) {
        keepAliveThread.join();
    }

    closesocket(udpSocket);
    //closesocket(udpBroadcastSocket);
    WSACleanup();
    std::cout << "Network closed.\n";
}

void GameLogic::init() {
    bool success = initNetwork();
    if (!success) {
        exit(1);
    }

    startNetworkThread();  // Start receiving data

    // Score Init
    font.loadFromFile("Arial Italic.ttf");

    player_score_text.setFont(font);
    player_score_text.setCharacterSize(30);

    player_leaderboard.setFont(font);
    player_leaderboard.setCharacterSize(30);


    game_over_text.setFont(font);
    game_over_text.setFillColor(sf::Color::White);
    game_over_text.setPosition(sf::Vector2f(300.f, 50.f)); // Adjust as needed
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
        game_timer = 0; // Game lasts for 60 seconds (adjust as needed)
    }
}

void GameLogic::update(sf::RenderWindow& window, float delta_time) {

    //// TO BE MOVED TO SERVER
    //if (!is_game_over) {
    //    game_timer -= delta_time;
    //    if (game_timer <= 0.f) {
    //        gameOver(); 
    //    }
    //}

    window.clear();
    
    // Test commands

    if (is_game_over) {
        game_over_text.setString("Press Enter to Start a New Match");
        window.draw(game_over_text);
        int i = 0;


        for (auto& [score, player] : leaderboard) {
            if (i == 0) {
                player_leaderboard.setString("Player Leaderboard");
                player_leaderboard.setPosition(sf::Vector2f(SCREEN_WIDTH / 2.f - 150.f, 200.f));
                window.draw(player_leaderboard);
            }

            if (i >= 5) break;  // Show only the top 5 players

            player_leaderboard.setPosition(sf::Vector2f(SCREEN_WIDTH / 2.f - 150.f, 250.f + (i * 80.f)));
            player_leaderboard.setString(player + ": " + std::to_string(score));
            window.draw(player_leaderboard);

            if (i == 4) {
                std::string name;
                if (playersNames.find(winner_id) != playersNames.end()) {
                    name = playersNames[winner_id];
                }
                else {
                    name = "Player " + std::to_string(winner_id);

                }
                player_leaderboard.setString(name + " Won! Score: " + std::to_string(winner_score));
                player_leaderboard.setPosition(sf::Vector2f(SCREEN_WIDTH / 2.f - 250.f, 700.f));
                window.draw(player_leaderboard);
            }

            i++;
        }

        // Wait for Enter key to restart
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter) && window.hasFocus()) {
            //entities.clear();
     
            // Test player
            //Player* new_player = new Player(current_session_id + 1, player_colors[current_session_id + 1], sf::Vector2f(500.f, 400.f), 30.f);
            //players[current_session_id + 1] = new_player; // Store in map
            //entities.push_back(new_player);

            std::cout << "Sending REQ_START_GAME" << std::endl;
            std::vector<char> conn_buffer = { REQ_START_GAME }; 
            sendData(conn_buffer);

        }


    }
    else {

        {
            std::vector<char> buffer;  // Start empty
            bool useReliableSender = false;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && window.hasFocus()) {
                //std::cout << "Rotating" << std::endl;


                buffer.push_back(SELF_SPACESHIP);  // Packet type
                buffer.push_back(static_cast<char>(current_session_id));  // Session ID

                // Ensure player exists
                if (players.find(current_session_id) == players.end()) {
                    std::cerr << "Error: Player not found!" << std::endl;
                    return;
                }

                Player* player = players[current_session_id];

                // Append position (x, y)
                //std::vector<char> bytes = Global::t_to_bytes(player->position.x);
                //buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                //bytes = Global::t_to_bytes(player->position.y);
                //buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                // Append velocity (vector.x, vector.y)
                std::vector<char> bytes = Global::t_to_bytes(player->velocity.x);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                bytes = Global::t_to_bytes(player->velocity.y);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                // Append rotation
                player->angle -= (TURN_SPEED * delta_time);
                player->angle = std::fmod(player->angle + 360, 360);

                // Convert to bytes and append
                bytes = Global::t_to_bytes(player->angle);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());




                sendData(buffer);
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && window.hasFocus()) {
                buffer.push_back(SELF_SPACESHIP);  // Packet type
                buffer.push_back(static_cast<char>(current_session_id));  // Session ID

                // Ensure player exists
                if (players.find(current_session_id) == players.end()) {
                    std::cerr << "Error: Player not found!" << std::endl;
                    return;
                }

                Player* player = players[current_session_id];

                // Append velocity (vector.x, vector.y)
                std::vector<char> bytes = Global::t_to_bytes(player->velocity.x);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                bytes = Global::t_to_bytes(player->velocity.y);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                // Append rotation
                player->angle += (TURN_SPEED * delta_time);
                player->angle = std::fmod(player->angle + 360, 360);

                bytes = Global::t_to_bytes(player->angle);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());



                sendData(buffer);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && window.hasFocus()) {

                float radians = players[current_session_id]->angle * (M_PI / 180.f);

                buffer.push_back(SELF_SPACESHIP);  // Packet type
                buffer.push_back(static_cast<char>(current_session_id));  // Session ID

                // Ensure player exists
                if (players.find(current_session_id) == players.end()) {
                    std::cerr << "Error: Player not found!" << std::endl;
                    return;
                }

                Player* player = players[current_session_id];

                // Append position (x, y)
                //std::vector<char> bytes = Global::t_to_bytes(player->position.x);
                //buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                //bytes = Global::t_to_bytes(player->position.y);
                //buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                // Append velocity (vector.x, vector.y)
                //bytes = Global::t_to_bytes(players[current_session_id]->velocity.x + cos(radians) * (ACCELERATION * delta_time));
                std::vector<char> bytes = Global::t_to_bytes(players[current_session_id]->velocity.x += cos(radians) * (ACCELERATION * delta_time));
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                //bytes = Global::t_to_bytes(players[current_session_id]->velocity.y + sin(radians) * (ACCELERATION * delta_time));
                bytes = Global::t_to_bytes(players[current_session_id]->velocity.y += sin(radians) * (ACCELERATION * delta_time));
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                // Append rotation
                bytes = Global::t_to_bytes(players[current_session_id]->angle);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());



                sendData(buffer);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && window.hasFocus()) {

                float radians = players[current_session_id]->angle * (M_PI / 180.f);

                buffer.push_back(SELF_SPACESHIP);  // Packet type
                buffer.push_back(static_cast<char>(current_session_id));  // Session ID

                // Ensure player exists
                if (players.find(current_session_id) == players.end()) {
                    std::cerr << "Error: Player not found!" << std::endl;
                    return;
                }

                Player* player = players[current_session_id];
                auto& vel = players[current_session_id]->velocity;
                const float MAX_SPEED = 20.f; // or whatever value fits your game

                vel.x += cos(radians) * (ACCELERATION * delta_time);
                vel.y += sin(radians) * (ACCELERATION * delta_time);


                float speed = std::pow(vel.x * vel.x + vel.y * vel.y, 2);

                // Clamp the velocity if the speed exceeds the max speed
                if (speed > MAX_SPEED) {
                    float scale = MAX_SPEED / speed;
                    vel.x *= scale;
                    vel.y *= scale;
                }
                
                // Append velocity (vector.x, vector.y)
                std::vector<char> bytes = Global::t_to_bytes(vel.x);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                bytes = Global::t_to_bytes(vel.y);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                // Append rotation
                bytes = Global::t_to_bytes(players[current_session_id]->angle);
                buffer.insert(buffer.end(), bytes.begin(), bytes.end());



                sendData(buffer);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && window.hasFocus()) {
                static auto last_bullet_fired = std::chrono::high_resolution_clock::now();
                auto now = std::chrono::high_resolution_clock::now();

                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_bullet_fired).count();
                //std::cout << "Elapsed Time: " << elapsed << "ms\n";  // Debug log

                if (elapsed >= SHOT_DELAY) {

                    if (players.find(current_session_id) == players.end()) {
                        std::cerr << "Error: Player not found! (ID: " << current_session_id << ")\n";
                        return;
                    }

                    Player* player = players[current_session_id];

                    if (player->lives_left == 0) {
                        return;
                    }

                    useReliableSender = true;
                    last_bullet_fired = now;

                    float radians = players[current_session_id]->angle * (M_PI / 180.f);

                    buffer.clear();  // Ensure buffer is clean
                    buffer.push_back(NEW_BULLET);
                    buffer.push_back(static_cast<char>(current_session_id));

                    // Add sequence number
                    buffer.push_back((seq >> 24) & 0xff);
                    buffer.push_back((seq >> 16) & 0xff);
                    buffer.push_back((seq >> 8) & 0xff);
                    buffer.push_back((seq >> 0) & 0xff);
                    ++seq;

                    // Add position
                    auto bytes = Global::t_to_bytes(player->position.x);
                    buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                    bytes = Global::t_to_bytes(player->position.y);
                    buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                    // Add bullet velocity
                    bytes = Global::t_to_bytes(cos(radians) * BULLET_SPEED);
                    buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                    bytes = Global::t_to_bytes(sin(radians) * BULLET_SPEED);
                    buffer.insert(buffer.end(), bytes.begin(), bytes.end());

                    std::cout << "Bullet fired!\n";
                }
            }


            auto reliableSender = [buffer]() {
                int retries = 5;

                int seq_num = buffer[2] << 24 | buffer[3] << 16 | buffer[4] << 8 | buffer[5];

                while (--retries >= 0) {
                    // Send the exact-sized buffer
                    sendData(buffer);


                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                    {
                        std::lock_guard < std::mutex> aslock(acked_seq_mutex);
                        if (acked_seq.find(seq_num) != acked_seq.end()) {
                            // has been acked
                            acked_seq.erase(seq_num);
                            return;
                        }
                    }

                    // have not been acked
                }

                std::cerr << "Server did not send ACK_SELF_SPACESHIP for 5 seconds!" << std::endl;
                };

            if (useReliableSender) {
                std::lock_guard<std::mutex> tplock(Global::threadpool_mutex);
                Global::threadpool.push_back(std::async(std::launch::async, reliableSender));
            }

            applyEntityUpdates();

            for (size_t i = 0; i < entities.size(); i++) {
                //entities[i]->update(delta_time);
                entities[i]->render(window);
            }

            // Draw scoreboard based on num of players
            int i = 0;

            for (auto& [sessionID, player] : players) {
                std::string name;

                if (playersNames.find(sessionID) != playersNames.end()) {
                    name = playersNames[sessionID];
                }
                else {
                    name = "Player " + std::to_string(sessionID);
                }

                player_score_text.setPosition(sf::Vector2f(40.f, 40.f + (i * 80.f)));
                player_score_text.setString(
                    name +
                    "\nScore: " + std::to_string(player->score) +
                    "\nLives: " + std::to_string(player->lives_left)
                );
                player_score_text.setFillColor(player->player_color);
                window.draw(player_score_text);
                i++;
            }



        }
    }
 
    // Show timer on screen
    if (!is_game_over) {
        sf::Text timer_text;
        timer_text.setFont(font);
        timer_text.setCharacterSize(30);
        timer_text.setPosition(sf::Vector2f(SCREEN_WIDTH / 2 - 40.f, 20.f)); // Adjust position as needed
        timer_text.setString("Time: " + std::to_string(static_cast<int>(game_timer))); // Convert to int for display


        window.draw(timer_text);
    }
   

}
void GameLogic::applyEntityUpdates() {
    // Update players
    if (updatedEntities) {
        for (const auto& updatedPlayer : updatedPlayers) {
            if (players.count(updatedPlayer.sid)) {
                // Update existing player

                Player* p = players[updatedPlayer.sid];

                p->position = updatedPlayer.position;

                if (p->lives_left != updatedPlayer.lives_left) {
                    p->velocity = updatedPlayer.velocity;
                    p->lives_left = updatedPlayer.lives_left;
                }

                p->score = updatedPlayer.score;

                if (updatedPlayer.sid != current_session_id)
                    p->angle = updatedPlayer.angle;
            }
        }

        // Remove existing bullets & asteroids in a single pass
        entities.erase(std::remove_if(entities.begin(), entities.end(), [](Entity* e) {
            if (dynamic_cast<Bullet*>(e) || dynamic_cast<Asteroid*>(e)) {
                delete e;  // Free memory
                return true;
            }
            return false;
            }), entities.end());

        // Add updated bullets
        for (const auto& updatedBullet : updatedBullets) {
            entities.push_back(new Bullet(updatedBullet));
        }

        // Add updated asteroids
        for (const auto& updatedAsteroid : updatedAsteroids) {
            entities.push_back(new Asteroid(updatedAsteroid));
        }

        updatedEntities = false;
    }
}


void GameLogic::gameOver() {
    is_game_over = true;
    game_timer = 0;
    leaderboard.clear();
    for (auto& [sessionID, player] : players) {
        std::string name;

        if (playersNames.find(sessionID) != playersNames.end()) {
            name = playersNames[sessionID];
        }
        else {
            name = "Player " + std::to_string(sessionID);

        }
    }

}

