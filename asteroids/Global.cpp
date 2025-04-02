#include "Global.h"

// RANDOM GEN
std::random_device Global::rd;
std::mt19937 Global::rng(Global::rd());

// TO BE MOVED TO SERVER
float Global::randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

void Global::addToLeaderboard(std::map<int, std::string, std::greater<int>>& leaderboard, int score, const std::string& playerName)
{
    leaderboard[score] = playerName; // Adds or updates the player's score

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

