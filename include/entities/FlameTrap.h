#pragma once

#include "entities/Enemy.h"
#include <memory>
#include <vector>

class EnemyProjectile;

enum class FlameDirection {
    Left,
    Right,
    Up,
    Down
};

class FlameTrap : public Enemy {
public:
    FlameTrap(float x, float y, const EnemyStats& stats);

    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

    void updateFlame(float dt, std::vector<std::unique_ptr<EnemyProjectile>>& enemyProjectiles);

    FlameDirection getDirection() const { return direction; }
    void setDirection(FlameDirection dir);
    void cycleDirection();

    float getActiveDuration() const { return activeDuration; }
    void setActiveDuration(float value) { activeDuration = value; }

    float getInactiveDuration() const { return inactiveDuration; }
    void setInactiveDuration(float value) { inactiveDuration = value; }

    float getShotInterval() const { return shotInterval; }
    void setShotInterval(float value) { shotInterval = value; }

    float getProjectileSpeed() const { return projectileSpeed; }
    void setProjectileSpeed(float value) { projectileSpeed = value; }

    float getProjectileRange() const { return projectileRange; }
    void setProjectileRange(float value) { projectileRange = value; }

    bool isActive() const { return active; }

    void resetCycle();

private:
    void updateFlameState(float dt);
    sf::Vector2f getDirectionVector() const;

    FlameDirection direction;
    float activeDuration;
    float inactiveDuration;
    float shotInterval;
    float projectileSpeed;
    float projectileRange;

    float stateTimer;
    float shotTimer;
    bool active;
};


