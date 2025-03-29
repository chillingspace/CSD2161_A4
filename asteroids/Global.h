#pragma once
#include <random>

// GLOBAL CONSTANTS
constexpr float M_PI = 3.14159265358979323846f;
constexpr int SCREEN_WIDTH = 1200;
constexpr int SCREEN_HEIGHT = 900;

class Global {
public:
    static std::random_device rd;
    static std::mt19937 rng;

    static float randomFloat(float min, float max);
};
