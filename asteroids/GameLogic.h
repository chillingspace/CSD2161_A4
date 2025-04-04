/* Start Header
*****************************************************************/
/*!
\file GameLogic.h
\author Sean Gwee, 2301326
\par g.boonxuensean@digipen.edu
\date 1 Apr 2025
\brief
This file implements the GameLogic headers
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/


#pragma once
#include <vector>
#include <list>
#include <unordered_map>
#include "Entity.h"
#include "Player.h"

class GameLogic
{
	private:


	public:
		static void init();
		static void start();
		static void update(sf::RenderWindow& window, float delta_time);

		// Handle list of entities
		static std::vector<Entity*> entities;
		//static std::list<Entity*> entitiesToDelete;
		//static std::list<Entity*> entitiesToAdd;
		static std::unordered_map<uint8_t, Player*> players;
		
		static int game_timer;
		static bool is_game_over;

		static int score;
		
		static void applyEntityUpdates();

		static Player* findPlayerBySession(uint8_t sessionID);

		static bool checkCollision(Entity* a, Entity* b);

		static void gameOver();

		static void cleanUp();
};

void closeNetwork();


