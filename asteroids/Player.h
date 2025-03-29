#pragma once
#include "Entity.h"

// PLAYER VARIABLES
constexpr float TURN_SPEED = 200.f;
constexpr float PLAYER_SPEED = 300.f;
constexpr float SHOT_DELAY = 0.5f;
constexpr float DEATH_TIMER = 3.f;
constexpr float INVULNERABILITY_TIME = 1.f;

class Player : public Entity {
    private:
        sf::VertexArray vertices;
        float shot_timer;

    public:
        int uid;
        int score;
        bool is_alive;
        float death_timer;
        float invulnerability_timer;
        sf::Color player_color;

        // Player constructor
        Player(int uid, sf::Color player_color);

        // Update player
        void update(float delta_time) override;

        // Draw player
        void render(sf::RenderWindow& window) override;

        void death();

        void respawn();
};

