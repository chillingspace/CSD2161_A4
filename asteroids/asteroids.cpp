#include <SFML/Graphics.hpp>
#include <iostream>

constexpr float M_PI = 3.14159265358979323846f;

constexpr float TURN_SPEED = 200.f;
constexpr float PLAYER_SPEED = 100.f;

class Player {
    private:
        sf::VertexArray vertices;

    public:
        // Player constructor
        Player() : position(500.f,500.f), angle(0.f), vertices(sf::Quads, 4) {

            vertices[0].position = sf::Vector2f(20, 0);
            vertices[1].position = sf::Vector2f(-30, -20);
            vertices[2].position = sf::Vector2f(-15, 0);
            vertices[3].position = sf::Vector2f(-30, 20);


            for (size_t i = 0; i < vertices.getVertexCount(); i++) {
                vertices[i].color = sf::Color::White;
            }
        }

        void Update(float delta_time) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                angle -= TURN_SPEED * delta_time;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                angle += TURN_SPEED * delta_time;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
                float radians = angle * (M_PI / 180.f);
                position.x += cos(radians) * PLAYER_SPEED * delta_time;
                position.y += sin(radians) * PLAYER_SPEED * delta_time;
            }
        }

        // Draw player
        void Draw(sf::RenderWindow& window) {
            sf::Transform transform;
            transform.translate(position).rotate(angle);
            window.draw(vertices, transform);
        };

        sf::Vector2f position;
        float angle;
};

int main()
{
    // Create a window with 800x800 resolution
    sf::RenderWindow window(sf::VideoMode(1200, 900), "Asteroids", sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;

    Player player;

    std::cout << "SFML Window Created Successfully!\n";

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

        // Clear screen
        window.clear(sf::Color::Black);

        player.Update(delta_time);
        player.Draw(window);
        // Display the updated frame
        window.display();
    }

    std::cout << "SFML Window Closed.\n";
    return 0;
}
