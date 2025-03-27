#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <list>

constexpr float M_PI = 3.14159265358979323846f;

constexpr float TURN_SPEED = 200.f;
constexpr float PLAYER_SPEED = 100.f;
constexpr float BULLET_SPEED = 750.f;
constexpr float BULLET_LIFETIME = 3.f;
constexpr float SHOT_DELAY = 0.3f;

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
std::list<std::vector<Entity*>::iterator> entitiesToDelete{};
class Bullet : public Entity {
    private:
        sf::CircleShape shape;
        sf::Vector2f direction;
        float lifetime;
    public:
        Bullet(sf::Vector2f pos, sf::Vector2f dir) : shape(1.f), direction(dir), Entity(pos, 0.f), lifetime(BULLET_LIFETIME){}

        void update(float delta_time) override {
            lifetime -= delta_time;
            position += direction * BULLET_SPEED * delta_time;

            if (lifetime <= 0.f) {
                entitiesToDelete.push_back(std::find(entities.begin(), entities.end(), this));
            }
        }

        void render(sf::RenderWindow& window) override {
            window.draw(shape, sf::Transform().translate(position));
        };

};

class Player : public Entity {
    private:
        sf::VertexArray vertices;
        float shot_timer;

    public:
        // Player constructor
        Player() : Entity(sf::Vector2f(500.f, 500.f), 0.f), vertices(sf::Triangles, 3), shot_timer() {
            vertices[0].position = sf::Vector2f(20, 0);   // Tip of the ship
            vertices[1].position = sf::Vector2f(-20, -15); // Bottom left
            vertices[2].position = sf::Vector2f(-20, 15);  // Bottom right

            for (size_t i = 0; i < vertices.getVertexCount(); i++) {
                vertices[i].color = sf::Color::White;
            }
        }
      
        // Update bullet
        void update(float delta_time) override {
            shot_timer -= delta_time;

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
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shot_timer <= 0.f) {
                shot_timer = SHOT_DELAY;
                float radians = angle * (M_PI / 180.f);

                entities.push_back(new Bullet(position, sf::Vector2f(cos(radians), sin(radians))));
            }
        }

        // Draw player
        void render(sf::RenderWindow& window) override {
            
            window.draw(vertices, sf::Transform().translate(position).rotate(angle));
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

        entitiesToDelete.clear();
        // Clear screen
        window.clear(sf::Color::Black);

        for (size_t i = 0; i < entities.size(); i++) {
            entities[i]->update(delta_time);
            entities[i]->render(window);
        }

        for (const auto& it : entitiesToDelete) {
            delete* it;
            entities.erase(it);
        }

        // Display the updated frame
        window.display();
    }

    std::cout << "SFML Window Closed.\n";
    return 0;
}
