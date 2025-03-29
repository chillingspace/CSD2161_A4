#pragma once
#include <SFML/Graphics.hpp>
#include "Global.h"

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
    void wrapAround(sf::Vector2f& position, float size) {
        float offset = size * 2.5f;  // Half the size

        // Off-screen checks adjusted to ensure full exit
        if (position.x + offset < 0) {
            position.x = SCREEN_WIDTH - offset;
        }
        else if (position.x - size > SCREEN_WIDTH) {
            position.x = -offset;
        }
        if (position.y + offset < 0) {
            position.y = SCREEN_HEIGHT + offset;
        }
        else if (position.y - offset > SCREEN_HEIGHT) {
            position.y = -offset;
        }
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
