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


int main()
{
    // Create a window with 1600x900 resolution
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Asteroids", sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;
    std::cout << "Welcome to Spaceships!\n";

    GameLogic::init();
    std::thread t(Global::threadpoolManager);

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

    closeNetwork();
    GameLogic::cleanUp();
    Global::threadpool_running = false;
    t.join();

    std::cout << "SFML Window Closed.\n";

    return 0;

}
