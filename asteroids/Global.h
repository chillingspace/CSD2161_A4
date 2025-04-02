#pragma once
#include <random>
#include <iostream>
#include <vector>
#include <cstring>   // For memcpy
#include <winsock2.h> // For ntohl (Windows)
#include <type_traits>
#include <string>
#include <deque>
#include <future>
#include <mutex>

// GLOBAL CONSTANTS
constexpr float M_PI = 3.14159265358979323846f;
constexpr int SCREEN_WIDTH = 1600;
constexpr int SCREEN_HEIGHT = 900;

class Global {
public:
    static std::random_device rd;
    static std::mt19937 rng;

    static float randomFloat(float min, float max);


    /**
     * @file gay.cpp
     * @author your name (you@domain.com)
     * @brief
     * @version 0.1
     * @date 2025-04-02
     *
     * g++ .\gay.cpp -lws2_32
     *
     * @copyright Copyright (c) 2025
     *
     */



    template <typename T>
    static std::vector<char> t_to_bytes(T num) {
        std::vector<char> bytes(sizeof(T));

        if (std::is_integral<T>::value) {
            // for ints, convert to network byte order
            num = htonl(num);
            std::memcpy(bytes.data(), &num, sizeof(T));
        }
        else if (std::is_floating_point<T>::value) {
            // For floats
            unsigned int int_rep = *reinterpret_cast<unsigned int*>(&num);  // reinterpret float as unsigned int
            int_rep = htonl(int_rep);
            std::memcpy(bytes.data(), &int_rep, sizeof(T));
        }

        return bytes;
    }

    static float btof(const std::vector<char>& bytes);

    /* thread management */

    static bool threadpool_running;

    static std::deque<std::future<void>> threadpool;
    static std::mutex threadpool_mutex;

    void threadpoolManager();
};
