/* Start Header
*****************************************************************/
/*!
\file Asteroids.h
\author Sean Gwee, 2301326
\par g.boonxuensean@digipen.edu
\date 1 Apr 2025
\brief
This file implements the asteroid headers
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header
*******************************************************************/


#pragma once

#include "Entity.h"

// ASTEROIDS VARIABLES
constexpr float ASTEROID_SPEED = 80.f;
constexpr float ASTEROID_SPIN = 50.f;

class Asteroid : public Entity {
    private:

        sf::Vector2f velocity;


    public:
        sf::ConvexShape shape;
        float radius;

        // Asteroid constructor
        Asteroid();
        
        Asteroid(const Asteroid& asteroid);

        // Update Asteroid
        void update(float delta_time) override;

        // Draw Asteroid
        void render(sf::RenderWindow& window) override;

        void setRadius(float newRadius);

        void spawnOnEdge();
};
