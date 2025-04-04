/* Start Header
*****************************************************************/
/*!
\file main.cpp
\author Sean Gwee, 2301326
\par g.boonxuensean@digipen.edu
\date 1 Apr 2025
\brief
This file is the entry point to the game
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/



#include <SFML/Graphics.hpp>
#include <iostream>
#include <crtdbg.h>
#include "GameLogic.h"


//#define _CRTDBG_MAP_ALLOC
//#include <cstdlib>
//#include <crtdbg.h>

int main()
{
<<<<<<< Updated upstream
    // Create a window with 1600x900 resolution
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Asteroids", sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;
    std::cout << "Welcome to Spaceships!\n";
=======
	//#ifdef _DEBUG
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
	//#endif

	std::thread t(Global::threadpoolManager);
	{
		// Create a window with 1600x900 resolution
		sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Asteroids", sf::Style::Close | sf::Style::Titlebar);
		sf::Clock clock;
		std::cout << "SFML Window Created Successfully!\n";
>>>>>>> Stashed changes

		GameLogic::init();

		// Main loop
		while (window.isOpen())
		{
			float delta_time = clock.restart().asSeconds();
			sf::Event event;


			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed) // Close window on exit event
					window.close();
			}

			GameLogic::update(window, delta_time);

			window.display();
		}
	}

	closeNetwork();
	GameLogic::cleanUp();
	Global::threadpool_running = false;
	t.join();

<<<<<<< Updated upstream
    return 0;
=======
	std::cout << "SFML Window Closed.\n";

	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	return 0;
>>>>>>> Stashed changes

}
