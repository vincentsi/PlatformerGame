#pragma once

#include <SFML/Graphics.hpp>

namespace CollisionSystem {
    // AABB collision detection
    bool checkCollision(const sf::FloatRect& a, const sf::FloatRect& b);

    // Resolve collision and return true if collision happened
    bool resolveCollision(sf::FloatRect& movingRect, sf::Vector2f& velocity,
                          const sf::FloatRect& staticRect, bool& isGrounded);
}
