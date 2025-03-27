#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <random>
// GLOBAL VARIABLES
constexpr float M_PI = 3.14159265358979323846f;
constexpr int SCREEN_WIDTH = 1200;
constexpr int SCREEN_HEIGHT = 900;

// PLAYER VARIABLES
constexpr float TURN_SPEED = 200.f;
constexpr float PLAYER_SPEED = 100.f;
constexpr float BULLET_SPEED = 750.f;
constexpr float BULLET_LIFETIME = 3.f;
constexpr float SHOT_DELAY = 0.3f;

// ASTEROIDS VARIABLES
constexpr float ASTEROID_SPEED = 80.f;
constexpr float ASTEROID_SPIN = 50.f;

// RANDOM GEN
std::random_device rd;
std::mt19937 rng(rd());

float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

class Entity {
    private:

    public:
        Entity(sf::Vector2f pos, float angle) : position(pos), angle(angle) {};
        virtual void update(float delta_time) = 0;
        virtual void render(sf::RenderWindow& window) = 0;

        void wrapAround(sf::Vector2f& position) {
            if (position.x < 0) position.x += SCREEN_WIDTH;
            if (position.x > SCREEN_WIDTH) position.x -= SCREEN_WIDTH;
            if (position.y < 0) position.y += SCREEN_HEIGHT;
            if (position.y > SCREEN_HEIGHT) position.y -= SCREEN_HEIGHT;
        }

        // Draw with wraparound
        template <typename T>
        void drawWithWraparound(sf::RenderWindow& window, T& drawable, sf::Vector2f position, float rotation) {
            sf::Transform transform;
            transform.translate(position).rotate(rotation);

            window.draw(drawable, transform);

            // Draw additional instances near edges to create the illusion of wrapping
            if (position.x < 50) {
                window.draw(drawable, sf::Transform().translate(position.x + SCREEN_WIDTH, position.y).rotate(rotation));
            }
            if (position.x > SCREEN_WIDTH - 50) {
                window.draw(drawable, sf::Transform().translate(position.x - SCREEN_WIDTH, position.y).rotate(rotation));
            }
            if (position.y < 50) {
                window.draw(drawable, sf::Transform().translate(position.x, position.y + SCREEN_HEIGHT).rotate(rotation));
            }
            if (position.y > SCREEN_HEIGHT - 50) {
                window.draw(drawable, sf::Transform().translate(position.x, position.y - SCREEN_HEIGHT).rotate(rotation));
            }
        }

        sf::Vector2f position;
        float angle;
};

// Handle list of entities
std::vector<Entity*> entities{};
std::list<std::vector<Entity*>::iterator> entitiesToDelete{};
std::list<Entity*> entitiesToAdd{};

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
        
        // Update player
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

            wrapAround(position);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shot_timer <= 0.f) {
                shot_timer = SHOT_DELAY;
                float radians = angle * (M_PI / 180.f);


                entitiesToAdd.push_back(new Bullet(position, sf::Vector2f(cos(radians), sin(radians))));
            }
        }

        // Draw player
        void render(sf::RenderWindow& window) override {

            //window.draw(vertices, sf::Transform().translate(position).rotate(angle));
            drawWithWraparound(window, vertices, position, angle);
        };

};

class Asteroid : public Entity {
private:
    sf::ConvexShape shape;
    sf::Vector2f velocity;
    float size;

public:
    // Asteroid constructor
    Asteroid() : Entity(sf::Vector2f(0, 0), 0.f) {
        //size = randomFloat(20.f, 60.f); // Random size between 20 and 60
        spawnOnEdge();
        size = 20.f;
        velocity = sf::Vector2f(randomFloat(-1.f, 1.f), randomFloat(-1.f, 1.f)); // Random velocity

        shape.setPointCount(8); // 8-sided asteroid
        //for (int i = 0; i < 8; i++) {
        //    float angle = (i / 8.f) * 360.f;
        //    float radius = randomFloat(size * 0.8f, size); // Random radius per vertex
        //    float rad = angle * (M_PI / 180.f);
        //    shape.setPoint(i, sf::Vector2f(cos(rad) * radius, sin(rad) * radius));
        //}
        
        shape.setPoint(0, sf::Vector2f(40, -20));
        shape.setPoint(1, sf::Vector2f(20, -40));
        shape.setPoint(2, sf::Vector2f(-10, -50));
        shape.setPoint(3, sf::Vector2f(-40, -30));
        shape.setPoint(4, sf::Vector2f(-50, 0));
        shape.setPoint(5, sf::Vector2f(-30, 40));
        shape.setPoint(6, sf::Vector2f(10, 50));
        shape.setPoint(7, sf::Vector2f(35, 30));

        shape.setFillColor(sf::Color::Transparent); // Make it look like an outline
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(2);

    }


    // Update Asteroid
    void update(float delta_time) override {


        angle += ASTEROID_SPIN * delta_time;
        position += ASTEROID_SPEED * velocity * delta_time ;
        wrapAround(position);
    }

    // Draw Asteroid
    void render(sf::RenderWindow& window) override {

        /*window.draw(shape, sf::Transform().translate(position).rotate(angle));*/
        drawWithWraparound(window, shape, position, angle);
    };

    void spawnOnEdge() {
        int edge = rand() % 4; // 0 = top, 1 = bottom, 2 = left, 3 = right

        if (edge == 0) { // Top edge
            position = sf::Vector2f(randomFloat(0, SCREEN_WIDTH), 0);
            velocity = sf::Vector2f(randomFloat(-1.f, 1.f), randomFloat(0.5f, 1.f)); // Move downward
        }
        else if (edge == 1) { // Bottom edge
            position = sf::Vector2f(randomFloat(0, SCREEN_WIDTH), SCREEN_HEIGHT);
            velocity = sf::Vector2f(randomFloat(-1.f, 1.f), randomFloat(-1.f, -0.5f)); // Move upward
        }
        else if (edge == 2) { // Left edge
            position = sf::Vector2f(0, randomFloat(0, SCREEN_HEIGHT));
            velocity = sf::Vector2f(randomFloat(0.5f, 1.f), randomFloat(-1.f, 1.f)); // Move right
        }
        else { // Right edge
            position = sf::Vector2f(SCREEN_WIDTH, randomFloat(0, SCREEN_HEIGHT));
            velocity = sf::Vector2f(randomFloat(-1.f, -0.5f), randomFloat(-1.f, 1.f)); // Move left
        }

        velocity *= ASTEROID_SPEED; // Scale velocity
    }

};


int main()
{
    // Create a window with 800x800 resolution
    sf::RenderWindow window(sf::VideoMode(1200, 900), "Asteroids", sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;


    entitiesToAdd.push_back(new Player());

    for (size_t i = 0; i < 4; i++) {
        entitiesToAdd.push_back(new Asteroid());
    }
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

        for (const auto& entity : entitiesToDelete) {
            delete* entity;
            entities.erase(entity);
        }
        entitiesToDelete.clear();

        for (auto& entity : entitiesToAdd) {
            entities.push_back(entity);
        }
        entitiesToAdd.clear();

        // Display the updated frame
        window.display();
    }

    std::cout << "SFML Window Closed.\n";
    return 0;
}
