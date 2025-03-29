#pragma once
#include "Entity.h"

// PLAYER VARIABLES
constexpr float TURN_SPEED = 200.f;
constexpr float PLAYER_SPEED = 300.f;
constexpr float SHOT_DELAY = 0.5f;

class Player : public Entity {
    private:
        sf::VertexArray vertices;
        float shot_timer;

    public:
        // Player constructor
        Player();

        // Update player
        void update(float delta_time) override;

        // Draw player
        void render(sf::RenderWindow& window) override;

};

