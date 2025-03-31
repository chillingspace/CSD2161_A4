#include "Global.h"

// RANDOM GEN
std::random_device Global::rd;
std::mt19937 Global::rng(Global::rd());

// TO BE MOVED TO SERVER
float Global::randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}
