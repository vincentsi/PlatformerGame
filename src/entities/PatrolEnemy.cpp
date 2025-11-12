#include "entities/PatrolEnemy.h"

PatrolEnemy::PatrolEnemy(float x, float y, float patrolDistance)
    : Enemy(x, y, EnemyType::Patrol)
    , direction(Direction::Right)
{
    speed = 80.0f;
    setPatrolBounds(x - patrolDistance / 2.0f, x + patrolDistance / 2.0f);
}

void PatrolEnemy::update(float dt) {
    if (!alive) return;

    // Move based on direction
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

    // Update shape position
    shape.setPosition(position);
}
