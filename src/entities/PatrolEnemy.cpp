#include "entities/PatrolEnemy.h"

PatrolEnemy::PatrolEnemy(float x, float y, float patrolDistance, const EnemyStats& stats)
    : Enemy(x, y, EnemyType::Patrol, stats)
    , direction(Direction::Right)
{
    setPatrolBounds(x - patrolDistance / 2.0f, x + patrolDistance / 2.0f);
}

void PatrolEnemy::update(float dt) {
    if (!alive) return;
    
    // Call base class update to handle shoot timer
    Enemy::update(dt);

    // Move based on direction
    if (direction == Direction::Right) {
        position.x += stats.speed * dt;

        // Check if reached right bound
        if (position.x >= patrolRightBound) {
            position.x = patrolRightBound;
            direction = Direction::Left;
        }
    } else {
        position.x -= stats.speed * dt;

        // Check if reached left bound
        if (position.x <= patrolLeftBound) {
            position.x = patrolLeftBound;
            direction = Direction::Right;
        }
    }

    // Update shape position
    shape.setPosition(position);
}
