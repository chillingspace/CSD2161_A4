/* Start Header
*****************************************************************/
/*!
\file Global.cpp
\author Sean Gwee, 2301326
\par g.boonxuensean@digipen.edu
\date 1 Apr 2025
\brief
This file implements the global game functions
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/


#include "Global.h"

// RANDOM GEN
std::random_device Global::rd;
std::mt19937 Global::rng(Global::rd());

// TO BE MOVED TO SERVER
float Global::randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

void Global::addToLeaderboard(std::map<int, LeaderboardEntry, std::greater<int>>& leaderboard, int score, const LeaderboardEntry& data)
{
    leaderboard[score] = data; // Adds or updates the player's score

    // If there are more than 5 entries, remove the lowest score
    if (leaderboard.size() > 5) {
        leaderboard.erase(std::prev(leaderboard.end())); // Remove the lowest score
    }
}


float Global::btof(const std::vector<char>& bytes) {
    if (bytes.size() != sizeof(float)) {
        throw std::invalid_argument("Invalid byte size for float conversion");
    }

    unsigned int int_rep;
    std::memcpy(&int_rep, bytes.data(), sizeof(float));
    int_rep = ntohl(int_rep);  // Convert from network byte order

    float num;
    std::memcpy(&num, &int_rep, sizeof(float));
    return num;
}

/* thread management */

bool Global::threadpool_running = true;
std::deque<std::future<void>> Global::threadpool;
std::mutex Global::threadpool_mutex;

void Global::threadpoolManager() {
    while (threadpool_running) {
        std::vector<std::future<void>> pool;
        {
            std::lock_guard<std::mutex> lock(threadpool_mutex);
            pool.insert(pool.end(), std::make_move_iterator(threadpool.begin()), std::make_move_iterator(threadpool.end()));
            threadpool.clear();
        }


        for (auto& fut : pool) {
            fut.get(); // waits till thread has completed
        }

        // at this point, all threads have completed
    }
}


//// TO BE MOVED TO SERVER
//bool GameLogic::checkCollision(Entity* a, Entity* b) {
//    // Compute the distance between the two entities
//    float distance = sqrt(pow(a->position.x - b->position.x, 2) +
//        pow(a->position.y - b->position.y, 2));
//
//    // Determine radius (approximate using bounding boxes)
//    float a_radius = 0.f, b_radius = 0.f;
//
//    if (Asteroid* asteroid = dynamic_cast<Asteroid*>(a)) {
//        a_radius = std::max(asteroid->shape.getLocalBounds().width,
//            asteroid->shape.getLocalBounds().height) / 2.f;
//    }
//    else if (Player* player = dynamic_cast<Player*>(a)) {
//        a_radius = 20.f; // Approximate player size (adjust based on your ship size)
//    }
//    else if (Bullet* bullet = dynamic_cast<Bullet*>(a)) {
//        a_radius = 5.f;  // Small bullet size
//    }
//
//    if (Asteroid* asteroid = dynamic_cast<Asteroid*>(b)) {
//        b_radius = std::max(asteroid->shape.getLocalBounds().width,
//            asteroid->shape.getLocalBounds().height) / 2.f;
//    }
//    else if (Player* player = dynamic_cast<Player*>(b)) {
//        b_radius = 20.f; // Approximate player size
//    }
//    else if (Bullet* bullet = dynamic_cast<Bullet*>(b)) {
//        b_radius = 5.f;  // Small bullet size
//    }
//
//    // Collision check (distance must be smaller than sum of radii)
//    return distance < (a_radius + b_radius);
//}

// Handles every thing that all players need to know
//void listenForBroadcast() {
//    if (udpBroadcastSocket == -1) {
//        udpBroadcastSocket = socket(AF_INET, SOCK_DGRAM, 0);
//        if (udpBroadcastSocket == INVALID_SOCKET) {
//            std::cerr << "Socket creation failed.\n";
//            WSACleanup();
//            exit(1);
//        }
//        std::cout << "UDP broadcast socket created successfully with port\n";
//
//        u_long mode = 1;
//        ioctlsocket(udpBroadcastSocket, FIONBIO, &mode);  // Set to non-blocking mode
//
//        serverBroadcastAddr.sin_family = AF_INET;
//        serverBroadcastAddr.sin_port = htons(udpBroadcastPort);
//        // 3001
//#ifdef LOCALHOST_DEV
//        const char* ip = "127.0.0.1";   // localhost addr
//        if (inet_pton(AF_INET, ip, &serverBroadcastAddr.sin_addr) <= 0) {
//            std::cerr << "Invalid address/Address not supported\n";
//        }
//#else
//        serverBroadcastAddr.sin_addr.s_addr = INADDR_ANY;       // !TODO: need to test on multiple devices
//#endif
//
//        if (bind(udpBroadcastSocket, (sockaddr*)&serverBroadcastAddr, sizeof(serverBroadcastAddr)) == SOCKET_ERROR) {
//            std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
//            closesocket(udpBroadcastSocket);
//            WSACleanup();
//            exit(1);
//        }
//
//        std::cout << "UDP broadcast socket bind success" << std::endl;
//    }
//
//    std::cout << "Listening for UDP broadcasts on port " << serverBroadcastAddr.sin_port << "..." << std::endl;
//
//    while (isRunning) {
//        char buffer[1024];
//        sockaddr_in senderAddr;
//        int senderAddrSize = sizeof(senderAddr);
//
//        int bytesReceived = recvfrom(udpBroadcastSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&senderAddr, &senderAddrSize);
//        if (bytesReceived > 0) {
//
//            char cmd = buffer[0];
//            std::vector<char> send_buffer;
//
//            switch (cmd) {
//                case START_GAME:
//                {
//
//                    if (GameLogic::is_game_over) {
//                        GameLogic::start();
//                        std::cout << "starting" << std::endl;
//                    }
//
//
//                    int offset = 2;
//                    for (int i{}; i < buffer[1]; i++) {     // iterate through num players
//                        int sid = buffer[offset++];
//                        int playernamesize = buffer[offset++];
//                        std::string playerName;
//                        for (int j{}; j < playernamesize; j++) {    // iterate through num chars in player name
//                            playerName += buffer[offset + j];
//                        }
//                        playersNames[sid] = playerName;
//
//                        offset += playernamesize;
//                    }
//
//                    // Example: Send ACK_START_GAME back to sender
//                    send_buffer = { ACK_START_GAME };
//                    sendData(send_buffer);
//                }
//                break;
//                case ALL_ENTITIES: {
//                    std::cout << "Received ALL_ENTITIES update (" << bytesReceived << " bytes).\n";
//
//                    if (bytesReceived < 2) {
//                        std::cerr << "Error: Packet too short for ALL_ENTITIES\n";
//                        break;
//                    }
//
//                    int offset = 1;  // Skip packet type
//                    int num_spaceships = buffer[offset++];
//                    updatedPlayers.clear();
//
//                    // Read Player Data (15 bytes per spaceship)
//                    for (int i = 0; i < num_spaceships; i++) {
//                        if (offset + 15 > bytesReceived) {
//                            std::cerr << "Error: Not enough bytes for spaceship data\n";
//                            break;
//                        }
//
//                        Player s;
//                        s.sid = buffer[offset++];
//                        s.position.x = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
//                        offset += 4;
//                        s.position.y = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
//                        offset += 4;
//                        s.angle = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
//                        offset += 4;
//                        s.lives_left = buffer[offset++];  // 1 byte for lives_left (no need for memcpy)
//                        s.score = buffer[offset++];  // 1 byte for score (no need for memcpy)
//                        s.player_color = player_colors[s.sid];
//
//                        updatedPlayers.push_back(s);
//                        updatedEntities = true;
//                        std::cout << "Spaceship SID: " << static_cast<int>(s.sid)
//                            << " | Pos: (" << s.position.x << ", " << s.position.y << ")"
//                            << " | Angle : " << s.angle
//                            << " | Lives: " << (int)s.lives_left
//                            << " | Score: " << (int)s.score << "\n";
//                    }
//
//                    int num_bullets = buffer[offset++];
//
//                    updatedBullets.clear();
//                    // Read Bullet Data (9 bytes per bullet)
//                    for (int i = 0; i < num_bullets; i++) {
//                        if (offset + 9 > bytesReceived) {
//                            std::cerr << "Error: Not enough bytes for bullet data\n";
//                            break;
//                        }
//
//                        Bullet b;
//                        b.sid = buffer[offset++];  
//                        b.position.x = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
//                        offset += 4;
//                        b.position.y = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
//                        offset += 4;
//                        b.setColor(player_colors[b.sid]);
//                        updatedBullets.push_back(b);
//                        updatedEntities = true;
//                        //std::cout << "Bullet Owner SID: " << b.sid
//                        //    << " | Pos: (" << b.position.x << ", " << b.position.y << ")\n";
//                    }
//
//                    int num_asteroids = buffer[offset++];
//
//                    updatedAsteroids.clear();
//
//                    // Read Asteroid Data (8 bytes per asteroid)
//                    for (int i = 0; i < num_asteroids; i++) {
//                        if (offset + 8 > bytesReceived) {
//                            std::cerr << "Error: Not enough bytes for asteroid data\n";
//                            break;
//                        }
//
//                        Asteroid a;
//
//                        a.position.x = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
//                        offset += sizeof(float);
//                        a.position.y = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
//                        offset += sizeof(float);
//                        a.radius = Global::btof(std::vector<char>(buffer + offset, buffer + offset + sizeof(float)));
//                        offset += sizeof(float);
//                        a.setRadius(a.radius);
//                        updatedAsteroids.push_back(a);
//                        updatedEntities = true;
//                        //std::cout << "Asteroid Pos: (" << a.position.x << ", " << a.position.y << ") Radius = " << a.radius << "\n";
//                    }
//
//
//                    break;
//                }
//
//                case ACK_NEW_BULLET:
//                    std::cout << "Received ACK_NEW_BULLET.\n";
//                    // Handle spaceship acknowledgment
//                    break;
//
//                case END_GAME:
//
//                    int offset = 1;
//                    winner_id = buffer[offset++];
//                    winner_score = buffer[offset++];
//                    uint8_t num_high_scores = buffer[offset++];
//                    
//                    for (int i = 0; i < num_high_scores; i++) {
//                        int score = buffer[offset++];
//                        int name_length = buffer[offset++];
//
//                        std::string name;
//                        for (int j = 0; j < name_length; ++j) {
//                            name.push_back(buffer[offset + j]);
//                        }
//                        offset += name_length;
//
//                        // Add to the leaderboard
//                        Global::addToLeaderboard(leaderboard, score, name);
//                    }
//
//                    GameLogic::gameOver();
//                    // Send ACK
//                    send_buffer = { ACK_END_GAME };
//                    sendData(send_buffer);
//                    break;
//
//            }
//
//            buffer[bytesReceived] = '\0';
//            //std::cout << "Received broadcast from " << SERVER_IP << ": " << buffer << std::endl;
//
//        }
//        else if (bytesReceived == -1) {
//            //std::cout << "Bytes received == -1: " << WSAGetLastError() << std::endl;
//        }
//    }
//
//    //closesocket(udpBroadcastSocket);
//    //WSACleanup();
//}

//// TO BE MOVED TO SERVER
//Player* GameLogic::findPlayerBySession(uint8_t sessionID)
//{
//    auto it = players.find(sessionID);
//    return (it != players.end()) ? it->second : nullptr;
//}
