#pragma once
#include "Entity.h"

// TO BE MOVED TO SERVER
// PLAYER VARIABLES
constexpr float TURN_SPEED = 200.f;
constexpr float ACCELERATION = 200.f; // Adjust as needed
constexpr float FRICTION = 0.995f;     // Lower = more drag

constexpr float SHOT_DELAY = 0.5f;
constexpr float DEATH_TIMER = 3.f;
constexpr float INVULNERABILITY_TIME = 1.f;



class Player : public Entity {
    private:
        sf::Vector2f velocity;
        sf::VertexArray vertices;
        float shot_timer;

    public:
        uint8_t uid;
        int score;
        bool is_alive;
        float death_timer;
        float invulnerability_timer;
        sf::Color player_color;

        // Player constructor
        Player(int uid, sf::Color player_color, sf::Vector2f pos, float rot);

        // Update player
        void update(float delta_time) override;

        // Draw player
        void render(sf::RenderWindow& window) override;

        void death();

        void respawn();
};

