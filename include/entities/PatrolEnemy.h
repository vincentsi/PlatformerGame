#pragma once

#include "entities/Enemy.h"

class PatrolEnemy : public Enemy {
public:
    PatrolEnemy(float x, float y, float patrolDistance = 150.0f, const EnemyStats& stats = EnemyStats());

    void update(float dt) override;

private:
    enum class Direction {
        Left,
        Right
    };

    Direction direction;
};
