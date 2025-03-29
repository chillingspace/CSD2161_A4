#pragma once
#include <vector>
#include <list>
#include "Entity.h"
#include "Player.h"

class GameLogic
{
	private:
		static float game_timer;
		static float asteroid_spawn_time;
		static bool is_game_over;

	public:
		static void init();
		static void start();
		static void update(sf::RenderWindow& window, float delta_time);

		// Handle list of entities
		static std::vector<Entity*> entities;
		static std::list<Entity*> entitiesToDelete;
		static std::list<Entity*> entitiesToAdd;
		static std::vector<Player*> players;

		static int score;

		static bool checkCollision(Entity* a, Entity* b);

		static void gameOver();
};

