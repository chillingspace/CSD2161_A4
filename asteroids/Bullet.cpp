#include "Bullet.h"
#include "GameLogic.h"
#include "Player.h"
Bullet::Bullet(sf::Vector2f pos, sf::Vector2f dir, Player* owner) : shape(1.f), direction(dir), Entity(pos, 0.f), lifetime(BULLET_LIFETIME), owner(owner) {}

void Bullet::update(float delta_time)
{
    lifetime -= delta_time;
    position += direction * BULLET_SPEED * delta_time;

    // Check for collision with each asteroid
    for (Entity* entity : GameLogic::entities) {
        Asteroid* asteroid = dynamic_cast<Asteroid*>(entity);
        if (asteroid && GameLogic::checkCollision(this, asteroid)) {
            // If a collision is detected, handle it

            // Destroy the bullet
            GameLogic::entitiesToDelete.push_back(this);

            // Destroy or break the asteroid
            GameLogic::entitiesToDelete.push_back(asteroid);

            if (owner) {
                owner->score += 10;
            }

            break; // Exit the loop once we find a collision
        }
    }

    if (lifetime <= 0.f) {
        GameLogic::entitiesToDelete.push_back(this);
    }
}

void Bullet::render(sf::RenderWindow& window)
{
    window.draw(shape, sf::Transform().translate(position));
};
