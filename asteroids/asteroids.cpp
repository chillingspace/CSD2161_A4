#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

constexpr float M_PI = 3.14159265358979323846f;

constexpr float TURN_SPEED = 200.f;
constexpr float PLAYER_SPEED = 100.f;
constexpr float BULLET_SPEED = 1000.f;

class Entity {
    private:

    public:
        Entity(sf::Vector2f pos, float angle) : position(pos), angle(angle) {};
        virtual void update(float delta_time) = 0;
        virtual void render(sf::RenderWindow& window) = 0;

        sf::Vector2f position;
        float angle;
};

std::vector<Entity*> entities{};

class Bullet : public Entity {
    private:
        sf::CircleShape shape;
        sf::Vector2f direction;
    public:
        Bullet(sf::Vector2f pos, sf::Vector2f dir) : shape(1.f), direction(dir), Entity(pos, 0.f){}

        void update(float delta_time) override {
            position += direction * BULLET_SPEED * delta_time;
        }

        void render(sf::RenderWindow& window) override {
            window.draw(shape, sf::Transform().translate(position));
        };

};

class Player : public Entity {
    private:
        sf::VertexArray vertices;

    public:
        // Player constructor
        Player() : Entity(sf::Vector2f(500.f, 500.f), 0.f), vertices(sf::Triangles, 3) {
            vertices[0].position = sf::Vector2f(20, 0);   // Tip of the ship
            vertices[1].position = sf::Vector2f(-20, -15); // Bottom left
            vertices[2].position = sf::Vector2f(-20, 15);  // Bottom right

            for (size_t i = 0; i < vertices.getVertexCount(); i++) {
                vertices[i].color = sf::Color::White;
            }
        }
      
        // Update bullet
        void update(float delta_time) override {
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
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                float radians = angle * (M_PI / 180.f);

                entities.push_back(new Bullet(position, sf::Vector2f(cos(radians), sin(radians))));
            }
        }

        // Draw player
        void render(sf::RenderWindow& window) override {
            sf::Transform transform;
            transform.translate(position).rotate(angle);
            window.draw(vertices, transform);
        };

};

int main()
{
    // Create a window with 800x800 resolution
    sf::RenderWindow window(sf::VideoMode(1200, 900), "Asteroids", sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;


    entities.push_back(new Player());
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

        for (size_t i = 0; i < entities.size(); i++) {
            entities[i]->update(delta_time);
            entities[i]->render(window);
        }

        // Display the updated frame
        window.display();
    }

    std::cout << "SFML Window Closed.\n";
    return 0;
}
