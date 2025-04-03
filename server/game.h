/* Start Header
*****************************************************************/
/*!
\file game.h
\author Poh Jing Seng, 2301363
\par jingseng.poh\@digipen.edu
\date 1 Apr 2025
\brief
This file declares the game handlers
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/

#pragma once

#ifndef __GAME_H__
#define __GAME_H__

#include "server.h"


class Game {
private:
	Game() = default;

public:
	static constexpr int WINDOW_WIDTH = 1600;
	static constexpr int WINDOW_HEIGHT = 900;

	static constexpr int NUM_START_LIVES = 3;
	static constexpr float BULLET_RADIUS = 10.f;
	static constexpr float SPACESHIP_RADIUS = 20.f;
	
	static constexpr int GAME_DURATION_S = 60;
	static constexpr int ASTEROID_SPAWN_INTERVAL_MS = 1000;

	static constexpr int MIN_ASTEROID_RADIUS = 20;
	static constexpr int MAX_ASTEROID_RADIUS = 50;
	static constexpr float ASTEROID_SPEED = 80.f;
	static constexpr int MAX_ASTEROIDS = 20;

	static Game& getInstance();

	bool gameRunning = false;		// for updateGame loop
	bool gameThreadRunning = true;	// for updateGame loop thread

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

		vec2 operator-(const vec2& rhs) const {
			return { x - rhs.x, y - rhs.y };
		}

		vec2 operator*(const float scalar) {
			return {
				x * scalar,
				y * scalar
			};
		}

		void operator*=(const float scalar) {
			x *= scalar;
			y *= scalar;
		}

		float lengthSq() {
			return x * x + y * y;
		}

		float length() {
			return sqrt(lengthSq());
		}

		void normalize() {
			x /= length();
			y /= length();
		}
	};


	struct Asteroid {
		vec2 pos{};
		vec2 vector{};
		float radius{};
	};

	struct Bullet : public Asteroid {
		int bullet_id{};
		SESSION_ID sid{};
		float radius{ BULLET_RADIUS };
	};

	struct Spaceship : public Bullet {
		float rotation{};
		int lives_left{};
		int score{};
		std::string name{};
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

		//void resetSpaceship(SESSION_ID sid);
		void killSpaceship(std::vector<Spaceship>::iterator& it);
	};

	Data data;
	std::mutex data_mutex;

	static constexpr const char* highscore_file = "highscores.txt";
	struct Highscore {
		std::string playername;
		int score;
		std::string datestring;
	};
	std::deque<Highscore> highscores;
	static constexpr int NUM_HIGHSCORES = 5;

	/**
	 * use on a separate thread.
	 * 
	 */
	void updateGame();


	struct Circle {
		vec2 pos;
		float radius;
	};

	bool circleCollision(Circle c1, Circle c2);
};

#endif // __GAME_H__
