#include "entities/Enemy.h"

Enemy::Enemy(float x, float y, EnemyType type, const EnemyStats& stats)
    : Entity(x, y, stats.sizeX, stats.sizeY)
    , type(type)
    , alive(true)
    , stats(stats)
    , currentHP(stats.maxHP)
    , shootTimer(0.0f)
    , patrolLeftBound(x - 100.0f)
    , patrolRightBound(x + 100.0f)
{
    // Set size and color from stats
    shape.setSize(sf::Vector2f(stats.sizeX, stats.sizeY));
    shape.setFillColor(stats.color);
    shape.setPosition(position);
}

void Enemy::draw(sf::RenderWindow& window) {
    if (alive) {
        window.draw(shape);
    }
}

void Enemy::kill() {
    alive = false;
    currentHP = 0;
}

void Enemy::takeDamage(int amount) {
    currentHP -= amount;
    if (currentHP <= 0) {
        currentHP = 0;
        kill();
    }
}

void Enemy::setPatrolBounds(float leftBound, float rightBound) {
    patrolLeftBound = leftBound;
    patrolRightBound = rightBound;
}

float Enemy::getPatrolDistance() const {
    return (patrolRightBound - patrolLeftBound);
}

void Enemy::setPatrolDistance(float distance) {
    float centerX = (patrolLeftBound + patrolRightBound) / 2.0f;
    patrolLeftBound = centerX - distance / 2.0f;
    patrolRightBound = centerX + distance / 2.0f;
}

void Enemy::setPosition(float x, float y) {
    Entity::setPosition(x, y);
    shape.setPosition(position);
}

void Enemy::setPosition(const sf::Vector2f& pos) {
    Entity::setPosition(pos);
    shape.setPosition(position);
}

void Enemy::update(float dt) {
    // Update shoot timer
    if (shootTimer > 0.0f) {
        shootTimer -= dt;
        if (shootTimer < 0.0f) {
            shootTimer = 0.0f;
        }
    }
}
