#include "Player.h"
#include "Bullet.h"
#include "Global.h"
#include "GameLogic.h"

Player::Player() : Entity(sf::Vector2f(0.f,0.f), 0) {}

Player::Player(uint8_t sid, sf::Color player_color, sf::Vector2f pos, float rot) : Entity(pos, rot), vertices(sf::Triangles, 3), sid(sid), score(0), is_alive(true), death_timer(0.f), invulnerability_timer(0.f), shot_timer(), player_color(player_color), velocity(sf::Vector2f(0.f,0.f)) {
    vertices[0].position = sf::Vector2f(20, 0);   // Tip of the ship
    vertices[1].position = sf::Vector2f(-20, -15); // Bottom left
    vertices[2].position = sf::Vector2f(-20, 15);  // Bottom right
    lives_left = 3;
    for (size_t i = 0; i < vertices.getVertexCount(); i++) {
        vertices[i].color = player_color;
    }
}

// TO BE MOVED TO SERVER
void Player::update(float delta_time)
{
    if (!is_alive) {
        death_timer -= delta_time;

        // Shrink gradually over time
        float scale_factor = std::max(0.1f, death_timer / DEATH_TIMER); // Prevents negative size
        vertices[0].position = sf::Vector2f(20 * scale_factor, 0);
        vertices[1].position = sf::Vector2f(-20 * scale_factor, -15 * scale_factor);
        vertices[2].position = sf::Vector2f(-20 * scale_factor, 15 * scale_factor);

        // Fade out by decreasing alpha gradually
        for (size_t i = 0; i < vertices.getVertexCount(); i++) {
            vertices[i].color.a = static_cast<sf::Uint8>(255 * (scale_factor)); // Decrease alpha proportionally to scale factor
        }

        if (death_timer <= 0.f) {
            respawn();
        }
        return; // Don't process input while dead
    }

    if (invulnerability_timer > 0.f) {
        invulnerability_timer -= delta_time;
    }

    shot_timer -= delta_time;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        angle -= TURN_SPEED * delta_time;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        angle += TURN_SPEED * delta_time;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        float radians = angle * (M_PI / 180.f);
        velocity.x += cos(radians) * ACCELERATION * delta_time;
        velocity.y += sin(radians) * ACCELERATION * delta_time;
    }

    // Apply gradual friction to slow down
    float friction_factor = 1.0f - (FRICTION * delta_time);
    friction_factor = std::max(friction_factor, 0.0f);  // Prevent negative scaling

    velocity.x *= friction_factor;
    velocity.y *= friction_factor;
    // Update position based on velocity
    position += velocity * delta_time;

    wrapAround(position);



    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shot_timer <= 0.f) {
        shot_timer = SHOT_DELAY;
        float radians = angle * (M_PI / 180.f);


        GameLogic::entitiesToAdd.push_back(new Bullet(position, sf::Vector2f(cos(radians), sin(radians)), this, this->sid));
    }
    // Check for collision with each asteroid
    for (Entity* entity : GameLogic::entities) {
        Asteroid* asteroid = dynamic_cast<Asteroid*>(entity);
        if (asteroid && GameLogic::checkCollision(this, asteroid) && invulnerability_timer <= 0.f) {
            // If a collision is detected, handle it
            death();
            // Destroy the asteroid
            GameLogic::entitiesToDelete.push_back(asteroid);

            break;
        }
    }
}

void Player::render(sf::RenderWindow& window)
{
    //window.draw(vertices, sf::Transform().translate(position).rotate(angle));
    drawWithWraparound(window, vertices, position, angle);
};

// TO BE MOVED TO SERVER
void Player::death() {
    is_alive = false;
    death_timer = DEATH_TIMER;
    invulnerability_timer = 0.f;
}

// TO BE MOVED TO SERVER
void Player::respawn() {
    is_alive = true;
    invulnerability_timer = INVULNERABILITY_TIME;

    position = sf::Vector2f(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f);
    angle = 0.f;
    velocity = sf::Vector2f(0.f, 0.f);

    vertices[0].position = sf::Vector2f(20, 0);   // Tip of the ship
    vertices[1].position = sf::Vector2f(-20, -15); // Bottom left
    vertices[2].position = sf::Vector2f(-20, 15);  // Bottom right
    for (size_t i = 0; i < vertices.getVertexCount(); i++) {
        vertices[i].color = player_color;
    }

}