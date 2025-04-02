#include "Global.h"

// RANDOM GEN
std::random_device Global::rd;
std::mt19937 Global::rng(Global::rd());

// TO BE MOVED TO SERVER
float Global::randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
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