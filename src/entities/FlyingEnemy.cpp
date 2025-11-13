#include "entities/FlyingEnemy.h"

FlyingEnemy::FlyingEnemy(float x, float y, float patrolDistance, bool horizontalPatrol)
    : Enemy(x, y, EnemyType::Flying)
    , direction(horizontalPatrol ? Direction::Right : Direction::Down)
    , isHorizontal(horizontalPatrol)
    , patrolTopBound(0.0f)
    , patrolBottomBound(0.0f)
{
    speed = 60.0f; // Slower than patrol enemy
    
    if (horizontalPatrol) {
        // Horizontal patrol: use Enemy's patrol bounds
        setPatrolBounds(x - patrolDistance / 2.0f, x + patrolDistance / 2.0f);
    } else {
        // Vertical patrol
        setVerticalPatrolBounds(y - patrolDistance / 2.0f, y + patrolDistance / 2.0f);
    }

    // Make it visually distinct (purple color, circular-ish)
    shape.setFillColor(sf::Color(150, 0, 255)); // Purple
    shape.setOutlineColor(sf::Color(200, 100, 255));
    shape.setOutlineThickness(2.0f);
}

void FlyingEnemy::update(float dt) {
    if (!alive) return;

    if (isHorizontal) {
        // Horizontal movement (left-right)
        if (direction == Direction::Right) {
            position.x += speed * dt;

            // Check if reached right bound
            if (position.x >= patrolRightBound) {
                position.x = patrolRightBound;
                direction = Direction::Left;
            }
        } else {
            position.x -= speed * dt;

            // Check if reached left bound
            if (position.x <= patrolLeftBound) {
                position.x = patrolLeftBound;
                direction = Direction::Right;
            }
        }
    } else {
        // Vertical movement (up-down)
        if (direction == Direction::Down) {
            position.y += speed * dt;

            // Check if reached bottom bound
            if (position.y >= patrolBottomBound) {
                position.y = patrolBottomBound;
                direction = Direction::Up;
            }
        } else {
            position.y -= speed * dt;

            // Check if reached top bound
            if (position.y <= patrolTopBound) {
                position.y = patrolTopBound;
                direction = Direction::Down;
            }
        }
    }

    // Update shape position
    shape.setPosition(position);
}

void FlyingEnemy::setVerticalPatrolBounds(float topBound, float bottomBound) {
    patrolTopBound = topBound;
    patrolBottomBound = bottomBound;
}
