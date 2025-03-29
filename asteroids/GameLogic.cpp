#include "GameLogic.h"
#include "Asteroid.h"
#include "Player.h"
#include "Bullet.h"

std::vector<Entity*> GameLogic::entities{};
std::list<Entity*> GameLogic::entitiesToDelete{};
std::list<Entity*> GameLogic::entitiesToAdd{};

float GameLogic::game_timer;
bool GameLogic::is_game_over;
int GameLogic::score;
float GameLogic::asteroid_spawn_time;

sf::Text GameLogic::game_over_text;
sf::Text GameLogic::score_text;
sf::Font GameLogic::font;

void GameLogic::init() {
    // Score Init
    font.loadFromFile("Arial Italic.ttf");
    score_text.setFont(font);
    score_text.setPosition(sf::Vector2f(40.f, 20.f));
    score_text.setCharacterSize(30);

    game_over_text.setFont(font);
    game_over_text.setFillColor(sf::Color::Red);
    game_over_text.setPosition(sf::Vector2f(200.f, 300.f)); // Adjust as needed
    game_over_text.setCharacterSize(60);
}

void GameLogic::start() {
    is_game_over = false;
    asteroid_spawn_time = ASTEROID_SPAWN_TIME;
    entitiesToAdd.push_back(new Player());
    entitiesToAdd.push_back(new Asteroid());
    game_timer = 10.f; // Game lasts for 60 seconds (adjust as needed)

}

void GameLogic::update(sf::RenderWindow& window, float delta_time) {
    if (!is_game_over) {
        game_timer -= delta_time;
        if (game_timer <= 0.f) {
            gameOver(); 
            entities.clear();
        }
    }

    window.clear();
    
    asteroid_spawn_time -= delta_time;

    for (auto& entity : entitiesToAdd) {
        entities.push_back(entity);
    }
    entitiesToAdd.clear();

    for (size_t i = 0; i < entities.size(); i++) {
        entities[i]->update(delta_time);
        entities[i]->render(window);
    }

    for (auto* entity : entitiesToDelete) {
        auto it = std::find(entities.begin(), entities.end(), entity);
        if (it != entities.end()) {
            delete* it;       // Delete the object
            entities.erase(it); // Remove from the list
        }
    }
    entitiesToDelete.clear();
    
    if (asteroid_spawn_time <= 0.f && entities.size() <= 10) {
        entities.push_back(new Asteroid());
        asteroid_spawn_time = ASTEROID_SPAWN_TIME;
    }

    score_text.setString("P1\nScore: " + std::to_string(score));
    // Show timer on screen
    sf::Text timer_text;
    timer_text.setFont(font);
    timer_text.setCharacterSize(30);
    timer_text.setPosition(sf::Vector2f(SCREEN_WIDTH / 2 - 40.f, 20.f)); // Adjust position as needed
    timer_text.setString("Time: " + std::to_string(static_cast<int>(game_timer))); // Convert to int for display


    window.draw(score_text);
    window.draw(timer_text);

    if (is_game_over) {
        game_over_text.setString("GAME OVER\nPress Enter to Restart");
        window.draw(game_over_text);

        // Wait for Enter key to restart
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
            entities.clear();
            start();
        }

       
    }

}

bool GameLogic::checkCollision(Entity* a, Entity* b) {
    // Compute the distance between the two entities
    float distance = sqrt(pow(a->position.x - b->position.x, 2) +
        pow(a->position.y - b->position.y, 2));

    // Determine radius (approximate using bounding boxes)
    float a_radius = 0.f, b_radius = 0.f;

    if (Asteroid* asteroid = dynamic_cast<Asteroid*>(a)) {
        a_radius = std::max(asteroid->shape.getLocalBounds().width,
            asteroid->shape.getLocalBounds().height) / 2.f;
    }
    else if (Player* player = dynamic_cast<Player*>(a)) {
        a_radius = 20.f; // Approximate player size (adjust based on your ship size)
    }
    else if (Bullet* bullet = dynamic_cast<Bullet*>(a)) {
        a_radius = 5.f;  // Small bullet size
    }

    if (Asteroid* asteroid = dynamic_cast<Asteroid*>(b)) {
        b_radius = std::max(asteroid->shape.getLocalBounds().width,
            asteroid->shape.getLocalBounds().height) / 2.f;
    }
    else if (Player* player = dynamic_cast<Player*>(b)) {
        b_radius = 20.f; // Approximate player size
    }
    else if (Bullet* bullet = dynamic_cast<Bullet*>(b)) {
        b_radius = 5.f;  // Small bullet size
    }

    // Collision check (distance must be smaller than sum of radii)
    return distance < (a_radius + b_radius);
}

void GameLogic::gameOver() {
    is_game_over = true;

}