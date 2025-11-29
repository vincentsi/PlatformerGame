#include "entities/FlameTrap.h"
#include "entities/EnemyProjectile.h"
#include <cmath>
#include <vector>

namespace {
const float DEFAULT_ACTIVE_DURATION = 1.5f;
const float DEFAULT_INACTIVE_DURATION = 1.5f;
const float DEFAULT_SHOT_INTERVAL = 0.2f;
const float DEFAULT_PROJECTILE_SPEED = 350.0f;
const float DEFAULT_PROJECTILE_RANGE = 450.0f;
}

FlameTrap::FlameTrap(float x, float y, const EnemyStats& trapStats)
    : Enemy(x, y, EnemyType::FlameTrap, trapStats)
    , direction(FlameDirection::Right)
    , activeDuration(DEFAULT_ACTIVE_DURATION)
    , inactiveDuration(DEFAULT_INACTIVE_DURATION)
    , shotInterval(DEFAULT_SHOT_INTERVAL)
    , projectileSpeed(DEFAULT_PROJECTILE_SPEED)
    , projectileRange(DEFAULT_PROJECTILE_RANGE)
    , stateTimer(0.0f)
    , shotTimer(0.0f)
    , active(false)
{
    shape.setFillColor(trapStats.color);
    shape.setOutlineColor(sf::Color::Yellow);
    shape.setOutlineThickness(2.0f);
    shape.setSize(sf::Vector2f(trapStats.sizeX, trapStats.sizeY));
    setDirection(direction);
}

void FlameTrap::setDirection(FlameDirection dir) {
    direction = dir;
    float width = stats.sizeX;
    float height = stats.sizeY;
    if (direction == FlameDirection::Up || direction == FlameDirection::Down) {
        shape.setSize(sf::Vector2f(height, width));
    } else {
        shape.setSize(sf::Vector2f(width, height));
    }
    size = shape.getSize();
}

void FlameTrap::cycleDirection() {
    switch (direction) {
        case FlameDirection::Left:  setDirection(FlameDirection::Right); break;
        case FlameDirection::Right: setDirection(FlameDirection::Up); break;
        case FlameDirection::Up:    setDirection(FlameDirection::Down); break;
        case FlameDirection::Down:  setDirection(FlameDirection::Left); break;
    }
}

void FlameTrap::update(float dt) {
    Enemy::update(dt);
    updateFlameState(dt);
}

void FlameTrap::draw(sf::RenderWindow& window) {
    // Change color if active
    sf::Color previous = shape.getFillColor();
    if (active) {
        shape.setFillColor(sf::Color(255, 200, 60));
    }
    Enemy::draw(window);
    shape.setFillColor(previous);
}

void FlameTrap::updateFlame(float dt, std::vector<std::unique_ptr<EnemyProjectile>>& enemyProjectiles) {
    if (!active) {
        return;
    }

    shotTimer -= dt;
    if (shotTimer <= 0.0f) {
        shotTimer = shotInterval;

        sf::Vector2f dirVec = getDirectionVector();
        if (dirVec == sf::Vector2f(0.0f, 0.0f)) {
            return;
        }

        sf::Vector2f spawnPos = getPosition();
        sf::Vector2f trapSize = getSize();
        spawnPos.x += trapSize.x * 0.5f;
        spawnPos.y += trapSize.y * 0.5f;

        spawnPos += dirVec * (trapSize.x * 0.5f + 10.0f);

        enemyProjectiles.push_back(std::make_unique<EnemyProjectile>(
            spawnPos,
            dirVec,
            projectileSpeed,
            projectileRange,
            getDamage()));
    }
}

void FlameTrap::updateFlameState(float dt) {
    stateTimer += dt;
    if (active) {
        if (stateTimer >= activeDuration) {
            active = false;
            stateTimer = 0.0f;
        }
    } else {
        if (stateTimer >= inactiveDuration) {
            active = true;
            stateTimer = 0.0f;
            shotTimer = 0.0f;
        }
    }
}

void FlameTrap::resetCycle() {
    stateTimer = 0.0f;
    shotTimer = 0.0f;
    active = false;
}

sf::Vector2f FlameTrap::getDirectionVector() const {
    switch (direction) {
        case FlameDirection::Left:  return sf::Vector2f(-1.0f, 0.0f);
        case FlameDirection::Right: return sf::Vector2f(1.0f, 0.0f);
        case FlameDirection::Up:    return sf::Vector2f(0.0f, -1.0f);
        case FlameDirection::Down:  return sf::Vector2f(0.0f, 1.0f);
    }
    return sf::Vector2f(0.0f, 0.0f);
}


