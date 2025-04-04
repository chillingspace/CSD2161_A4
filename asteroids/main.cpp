#include <SFML/Graphics.hpp>
#include <iostream>
#include <crtdbg.h>
#include "GameLogic.h"


int main()
{
    #ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
    #endif

    // Create a window with 1600x900 resolution
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Asteroids", sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;
    std::cout << "SFML Window Created Successfully!\n";

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
    Global::threadpool_running = false;
    t.join();

    std::cout << "SFML Window Closed.\n";

    _CrtDumpMemoryLeaks();

    return 0;



}
