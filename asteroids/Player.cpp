#include "Player.h"
#include "Bullet.h"
#include "Global.h"
#include "GameLogic.h"

Player::Player() : Entity(sf::Vector2f(500.f, 500.f), 0.f), vertices(sf::Triangles, 3), shot_timer() {
    vertices[0].position = sf::Vector2f(20, 0);   // Tip of the ship
    vertices[1].position = sf::Vector2f(-20, -15); // Bottom left
    vertices[2].position = sf::Vector2f(-20, 15);  // Bottom right

    for (size_t i = 0; i < vertices.getVertexCount(); i++) {
        vertices[i].color = sf::Color::White;
    }
}

void Player::update(float delta_time)
{
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


        GameLogic::entitiesToAdd.push_back(new Bullet(position, sf::Vector2f(cos(radians), sin(radians))));
    }
    // Check for collision with each asteroid
    for (Entity* entity : GameLogic::entities) {
        Asteroid* asteroid = dynamic_cast<Asteroid*>(entity);
        if (asteroid && GameLogic::checkCollision(this, asteroid)) {
            // If a collision is detected, handle it

            GameLogic::gameOver();

            // Destroy the asteroid
            GameLogic::entitiesToDelete.push_back(asteroid);
            // Destroy the player
            GameLogic::entitiesToDelete.push_back(this);



            break;
        }
    }
}

void Player::render(sf::RenderWindow& window)
{
    //window.draw(vertices, sf::Transform().translate(position).rotate(angle));
    drawWithWraparound(window, vertices, position, angle);
};
