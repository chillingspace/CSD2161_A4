#include <SFML/Graphics.hpp>
#include <iostream>
#include "GameLogic.h"


int main()
{
    // Create a window with 800x800 resolution
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Asteroids", sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;
    std::cout << "SFML Window Created Successfully!\n";

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

    std::cout << "SFML Window Closed.\n";
    return 0;
}
