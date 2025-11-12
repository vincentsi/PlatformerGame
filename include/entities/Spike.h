#pragma once

#include "entities/Enemy.h"

class Spike : public Enemy {
public:
    Spike(float x, float y);

    void update(float dt) override;
};
