#pragma once

#ifndef __GAME_H__
#define __GAME_H__

#include "server.h"

class Game {
private:
	Game() = default;

public:

	static Game& getInstance();

	// game stuff
	static constexpr int MAX_PLAYERS = 4;

	// for ALL_ENTITIES payload
	struct vec2 {
		union {
			struct { float x, y; };
			float raw[2];
		};

		vec2() : x{ 0 }, y{ 0 } {}
		vec2(float x, float y) : x{ x }, y{ y } {}

		vec2 operator+(const vec2& rhs) const {
			return vec2{ x + rhs.x, y + rhs.y };
		}

		void operator+=(const vec2& rhs) {
			x += rhs.x;
			y += rhs.y;
		}
	};


	struct Asteroid {
		vec2 pos{};
		vec2 vector{};
		float radius{};
	};

	static constexpr float BULLET_RADIUS = 5.f;
	struct Bullet : public Asteroid {
		SESSION_ID sid{};
		float radius{ BULLET_RADIUS };
	};

	struct Spaceship : public Bullet {
		float rotation{};
		int lives_left{};
		int score{};
	};

	class Data {
	public:
		Data() : last_updated{ std::chrono::high_resolution_clock::now() } {};

		std::vector<Spaceship> spaceships{};
		std::vector<Bullet> bullets{};
		std::vector<Asteroid> asteroids{};
		decltype(std::chrono::high_resolution_clock::now()) last_updated;

		std::vector<char> toBytes();

		void reset();
	};

	Data data;
	std::mutex data_mutex;


	void updateGame();
};

#endif // __GAME_H__
