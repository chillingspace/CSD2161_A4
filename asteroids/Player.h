#pragma once
#include "Entity.h"

// TO BE MOVED TO SERVER
// PLAYER VARIABLES
constexpr float TURN_SPEED = 270.f;
constexpr float ACCELERATION = 250.f; // Adjust as needed
constexpr float FRICTION = 0.95f;     // Lower = more drag

constexpr float SHOT_DELAY = 500.f;     // in milliseconds
//constexpr float DEATH_TIMER = 3.f;                                  // na. handled by server
//constexpr float INVULNERABILITY_TIME = 1.f;                       // not applicable. handled by server



class Player : public Entity {
    private:

        sf::VertexArray vertices;
        float shot_timer;

    public:
        uint8_t sid;
        int score;
        bool is_alive;
        int lives_left;
        float death_timer;
        float invulnerability_timer;
        sf::Vector2f velocity;
        sf::Color player_color;


        // Player constructor
        Player();

        Player(const Player& player);

        Player(uint8_t uid, sf::Color player_color, sf::Vector2f pos, float rot);

        // Update player
        void update(float delta_time) override;

        // Draw player
        void render(sf::RenderWindow& window) override;

        void death();

        void respawn();
};

