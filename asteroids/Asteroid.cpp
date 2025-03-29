#include "Asteroid.h"
#include "Global.h"

Asteroid::Asteroid() : Entity(sf::Vector2f(0, 0), 0.f) {
    //size = Global::randomFloat(20.f, 60.f); // Random size between 20 and 60
    spawnOnEdge();
    size = 20.f;
    velocity = sf::Vector2f(Global::randomFloat(-1.f, 1.f), Global::randomFloat(-1.f, 1.f)); // Random velocity

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

void Asteroid::update(float delta_time) {
    angle += ASTEROID_SPIN * delta_time;
    position += ASTEROID_SPEED * velocity * delta_time;
    wrapAround(position, size);

}

void Asteroid::render(sf::RenderWindow& window)
{
    window.draw(shape, sf::Transform().translate(position).rotate(angle));
    //drawWithWraparound(window, shape, position, angle);
}

void Asteroid::spawnOnEdge()
{
    int edge = rand() % 4; // 0 = top, 1 = bottom, 2 = left, 3 = right

    if (edge == 0) { // Top edge
        position = sf::Vector2f(Global::randomFloat(0, SCREEN_WIDTH), 0);
        velocity = sf::Vector2f(Global::randomFloat(-1.f, 1.f), Global::randomFloat(0.5f, 1.f)); // Move downward
    }
    else if (edge == 1) { // Bottom edge
        position = sf::Vector2f(Global::randomFloat(0, SCREEN_WIDTH), SCREEN_HEIGHT);
        velocity = sf::Vector2f(Global::randomFloat(-1.f, 1.f), Global::randomFloat(-1.f, -0.5f)); // Move upward
    }
    else if (edge == 2) { // Left edge
        position = sf::Vector2f(0, Global::randomFloat(0, SCREEN_HEIGHT));
        velocity = sf::Vector2f(Global::randomFloat(0.5f, 1.f), Global::randomFloat(-1.f, 1.f)); // Move right
    }
    else { // Right edge
        position = sf::Vector2f(SCREEN_WIDTH, Global::randomFloat(0, SCREEN_HEIGHT));
        velocity = sf::Vector2f(Global::randomFloat(-1.f, -0.5f), Global::randomFloat(-1.f, 1.f)); // Move left
    }

    velocity *= ASTEROID_SPEED; // Scale velocity
}

