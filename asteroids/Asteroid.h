#pragma once

#include "Entity.h"

// ASTEROIDS VARIABLES
constexpr float ASTEROID_SPEED = 80.f;
constexpr float ASTEROID_SPIN = 50.f;
constexpr float ASTEROID_SPAWN_TIME = 3.f;

class Asteroid : public Entity {
    private:

        sf::Vector2f velocity;
        float size;

    public:
        sf::ConvexShape shape;

        // Asteroid constructor
        Asteroid();


        // Update Asteroid
        void update(float delta_time) override;

        // Draw Asteroid
        void render(sf::RenderWindow& window) override;

        void spawnOnEdge();
};
