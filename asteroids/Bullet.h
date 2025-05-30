#pragma once
#include "Entity.h"
#include "Asteroid.h"
#include "Player.h"

// PLAYER VARIABLES
constexpr float BULLET_SPEED = 750.f;
constexpr float BULLET_LIFETIME = 2.f;      // shldnt be used


class Bullet : public Entity {
private:
    sf::CircleShape shape;
    sf::Vector2f direction;
    float lifetime;
public:
    uint8_t sid;

    sf::Color player_color;

    Bullet();

    Bullet(const Bullet& bullet);

    Bullet(sf::Vector2f pos, sf::Vector2f dir, uint8_t sid);

    void update(float delta_time) override;

    void render(sf::RenderWindow& window) override;

    void setColor(sf::Color color);
};

