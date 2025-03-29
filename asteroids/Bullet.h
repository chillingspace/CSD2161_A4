#pragma once
#include "Entity.h"
#include "Asteroid.h"

// PLAYER VARIABLES
constexpr float BULLET_SPEED = 750.f;
constexpr float BULLET_LIFETIME = 2.f;


class Bullet : public Entity {
private:
    sf::CircleShape shape;
    sf::Vector2f direction;
    float lifetime;
public:
    Bullet(sf::Vector2f pos, sf::Vector2f dir);

    void update(float delta_time) override;

    void render(sf::RenderWindow& window) override;
};

