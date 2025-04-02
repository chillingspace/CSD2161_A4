#include "Asteroid.h"
#include "Global.h"

Asteroid::Asteroid() : Entity(sf::Vector2f(0, 0), 0.f) {
    radius = 30.f; // Random size between 20 and 60
    spawnOnEdge();

    velocity = sf::Vector2f(Global::randomFloat(-1.f, 1.f), Global::randomFloat(-1.f, 1.f)); // Random velocity

    // Adjusting the shape points based on the size
    shape.setPointCount(8); // 8-sided asteroid


    for (int i = 0; i < 8; i++) {
        float angle = (i / 8.f) * 360.f;
        float rad = angle * (M_PI / 180.f);
        shape.setPoint(i, sf::Vector2f(cos(rad) * radius, sin(rad) * radius)); // Use randomRadius to set the point
    }

    shape.setFillColor(sf::Color::Transparent); // Make it look like an outline
    shape.setOutlineColor(sf::Color::White);
    shape.setOutlineThickness(2);
}

// Asteroid copy constructor
Asteroid::Asteroid(const Asteroid& asteroid)
    : Entity(asteroid),           // Copy the base class (Entity) data
    shape(asteroid.shape),       // Copy the shape (for rendering)
    velocity(asteroid.velocity), // Copy the velocity
    radius(asteroid.radius)          // Copy the size
{
}


// TO BE MOVED TO SERVER
void Asteroid::update(float delta_time) {
    angle += ASTEROID_SPIN * delta_time;
    //position += ASTEROID_SPEED * velocity * delta_time;
    //wrapAround(position, size);

}

void Asteroid::render(sf::RenderWindow& window)
{    

    window.draw(shape, sf::Transform().translate(position).rotate(angle));
    //drawWithWraparound(window, shape, position, angle);
}

void Asteroid::setRadius(float newRadius) {
    radius = newRadius;  // Update the class member radius

    // Adjust the shape to reflect the new size
    for (int i = 0; i < 8; i++) {
        float angle = (i / 8.f) * 360.f;
        float rad = angle * (M_PI / 180.f);
        shape.setPoint(i, sf::Vector2f(cos(rad) * radius, sin(rad) * radius)); // Use randomRadius to set the point
    }
}


// TO BE MOVED TO SERVER
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

