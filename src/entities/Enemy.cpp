#include "entities/Enemy.h"

Enemy::Enemy(float x, float y, EnemyType type)
    : Entity(x, y, 30.0f, 30.0f)
    , type(type)
    , alive(true)
    , speed(100.0f)
    , patrolLeftBound(x - 100.0f)
    , patrolRightBound(x + 100.0f)
{
    // Red color for enemies
    shape.setSize(sf::Vector2f(30.0f, 30.0f));
    shape.setFillColor(sf::Color::Red);
    shape.setPosition(position);
}

void Enemy::draw(sf::RenderWindow& window) {
    if (alive) {
        window.draw(shape);
    }
}

void Enemy::kill() {
    alive = false;
}

void Enemy::setPatrolBounds(float leftBound, float rightBound) {
    patrolLeftBound = leftBound;
    patrolRightBound = rightBound;
}
