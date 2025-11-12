#include "physics/CollisionSystem.h"
#include "physics/PhysicsConstants.h"
#include <cmath>

namespace CollisionSystem {

bool checkCollision(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.intersects(b);
}

bool resolveCollision(sf::FloatRect& movingRect, sf::Vector2f& velocity,
                      const sf::FloatRect& staticRect, bool& isGrounded)
{
    if (!checkCollision(movingRect, staticRect)) {
        return false;
    }

    // Calculate overlap on each axis
    float overlapLeft = (movingRect.left + movingRect.width) - staticRect.left;
    float overlapRight = (staticRect.left + staticRect.width) - movingRect.left;
    float overlapTop = (movingRect.top + movingRect.height) - staticRect.top;
    float overlapBottom = (staticRect.top + staticRect.height) - movingRect.top;

    // Find minimum overlap
    float minOverlapX = std::min(overlapLeft, overlapRight);
    float minOverlapY = std::min(overlapTop, overlapBottom);

    // Resolve collision on the axis with smallest overlap
    if (minOverlapX < minOverlapY) {
        // Horizontal collision
        if (overlapLeft < overlapRight) {
            // Collision on right side of static rect
            movingRect.left = staticRect.left - movingRect.width - Physics::EPSILON;
        } else {
            // Collision on left side of static rect
            movingRect.left = staticRect.left + staticRect.width + Physics::EPSILON;
        }
        velocity.x = 0.0f;
    } else {
        // Vertical collision
        if (overlapTop < overlapBottom) {
            // Collision on bottom side of static rect (player landing on platform)
            movingRect.top = staticRect.top - movingRect.height - Physics::EPSILON;
            velocity.y = 0.0f;
            isGrounded = true;
        } else {
            // Collision on top side of static rect (player hitting head)
            movingRect.top = staticRect.top + staticRect.height + Physics::EPSILON;
            velocity.y = 0.0f;
        }
    }

    return true;
}

} // namespace CollisionSystem
