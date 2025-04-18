#include "Bullet.h"
#include "GameLogic.h"
#include "Player.h"

Bullet::Bullet() : Entity(sf::Vector2f(0.f, 0.f), 0), shape(1.f), direction(sf::Vector2f(0.f, 0.f)), lifetime(BULLET_LIFETIME), player_color(sf::Color(255,255,255,255)), sid(0) {
   
};

Bullet::Bullet(const Bullet& bullet)
    : Entity(bullet),               // Copy the base class (Entity) data
    shape(bullet.shape),           // Copy the shape (for rendering)
    direction(bullet.direction),   // Copy the direction
    lifetime(bullet.lifetime),     // Copy lifetime
    sid(bullet.sid),               // Copy the SID
    player_color(bullet.player_color)            // Copy the owner pointer
{}

Bullet::Bullet(sf::Vector2f pos, sf::Vector2f dir, uint8_t sid) :Entity(pos, 0.f), shape(1.f), direction(dir),  lifetime(BULLET_LIFETIME), player_color(sf::Color(255, 255, 255, 255)), sid(sid) {

}

// TO BE MOVED TO SERVER
void Bullet::update(float delta_time)
{
    //lifetime -= delta_time;
    //position += direction * BULLET_SPEED * delta_time;

    //// Check for collision with each asteroid
    //for (Entity* entity : GameLogic::entities) {
    //    Asteroid* asteroid = dynamic_cast<Asteroid*>(entity);
    //    if (asteroid && GameLogic::checkCollision(this, asteroid)) {
    //        // If a collision is detected, handle it

    //        // Destroy the bullet
    //        GameLogic::entitiesToDelete.push_back(this);

    //        // Destroy or break the asteroid
    //        GameLogic::entitiesToDelete.push_back(asteroid);

    //        if (owner) {
    //            owner->score += 10;
    //        }

    //        break; // Exit the loop once we find a collision
    //    }
    //}

    if (lifetime <= 0.f) {
        //GameLogic::entitiesToDelete.push_back(this);
    }
}

void Bullet::render(sf::RenderWindow& window)
{
    shape.setPosition(position);
    shape.setRadius(4.f);
    window.draw(shape);
}
void Bullet::setColor(sf::Color color)
{
    shape.setFillColor(color);
}
;
