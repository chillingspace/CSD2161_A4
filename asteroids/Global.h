/* Start Header
*****************************************************************/
/*!
\file Global.h
\author Sean Gwee, 2301326
\par g.boonxuensean@digipen.edu
\date 1 Apr 2025
\brief
This file implements the global headers
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/


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
#include <map>

// GLOBAL CONSTANTS
constexpr float M_PI = 3.14159265358979323846f;
constexpr int SCREEN_WIDTH = 1600;
constexpr int SCREEN_HEIGHT = 900;

struct LeaderboardEntry {
    std::string playerName;
    std::string date;
};

class Global {
public:
    static std::random_device rd;
    static std::mt19937 rng;

    static float randomFloat(float min, float max);

    static void addToLeaderboard(std::map<int, LeaderboardEntry, std::greater<int>>& leaderboard, int score, const LeaderboardEntry& data);

    template <typename T>
    static std::vector<char> t_to_bytes(T num) {
        std::vector<char> bytes(sizeof(T));

        if constexpr (std::is_integral<T>::value) {
            if constexpr (sizeof(T) == 2) {
                uint16_t net = htons(static_cast<uint16_t>(num));
                std::memcpy(bytes.data(), &net, sizeof(net));
            }
            else if constexpr (sizeof(T) == 4) {
                uint32_t net = htonl(static_cast<uint32_t>(num));
                std::memcpy(bytes.data(), &net, sizeof(net));
            }
            else if constexpr (sizeof(T) == 1) {
                bytes[0] = static_cast<char>(num);
            }
            else {
                static_assert(sizeof(T) <= 4, "Unsupported integral size for network conversion");
            }
        }
        else if constexpr (std::is_floating_point<T>::value) {
            static_assert(sizeof(T) == 4, "Only 32-bit float supported");
            uint32_t int_rep = *reinterpret_cast<uint32_t*>(&num);
            int_rep = htonl(int_rep);
            std::memcpy(bytes.data(), &int_rep, sizeof(int_rep));
        }

        return bytes;
    }


    static float btof(const std::vector<char>& bytes);

    /* thread management */

    static bool threadpool_running;

    static std::deque<std::future<void>> threadpool;
    static std::mutex threadpool_mutex;

    static void threadpoolManager();
};
