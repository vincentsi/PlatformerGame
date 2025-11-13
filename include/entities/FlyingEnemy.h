#pragma once

#include "entities/Enemy.h"

class FlyingEnemy : public Enemy {
public:
    FlyingEnemy(float x, float y, float patrolDistance = 150.0f, bool horizontalPatrol = false);

    void update(float dt) override;

    // Flying-specific patrol bounds (vertical)
    void setVerticalPatrolBounds(float topBound, float bottomBound);
    float getTopBound() const { return patrolTopBound; }
    float getBottomBound() const { return patrolBottomBound; }

private:
    enum class Direction {
        Up,
        Down,
        Left,
        Right
    };

    Direction direction;
    bool isHorizontal;  // true = horizontal patrol, false = vertical patrol

    // Vertical patrol boundaries
    float patrolTopBound;
    float patrolBottomBound;
};
