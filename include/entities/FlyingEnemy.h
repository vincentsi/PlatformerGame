#pragma once

#include "entities/Enemy.h"

class FlyingEnemy : public Enemy {
public:
    FlyingEnemy(float x, float y, float patrolDistance = 150.0f);

    void update(float dt) override;

    // Flying-specific patrol bounds (vertical)
    void setVerticalPatrolBounds(float topBound, float bottomBound);
    float getTopBound() const { return patrolTopBound; }
    float getBottomBound() const { return patrolBottomBound; }

private:
    enum class Direction {
        Up,
        Down
    };

    Direction direction;

    // Vertical patrol boundaries
    float patrolTopBound;
    float patrolBottomBound;
};
