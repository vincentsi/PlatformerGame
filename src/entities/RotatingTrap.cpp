#include "entities/RotatingTrap.h"
#include <cmath>
#include <algorithm>

RotatingTrap::RotatingTrap(float x, float y, const EnemyStats& trapStats)
    : Enemy(x, y, EnemyType::RotatingTrap, trapStats)
    , rotationSpeed(120.0f)
    , angle(0.0f)
    , armLength(trapStats.sizeX)
    , armThickness(trapStats.sizeY)
    , boundingRadius(trapStats.sizeX * 0.5f)
    , pivot(x, y)
{
    stats.sizeX = armLength;
    stats.sizeY = armThickness;
    size = sf::Vector2f(boundingRadius * 2.0f, boundingRadius * 2.0f);
    position = pivot;
    shape.setSize(sf::Vector2f(armLength, armThickness));
    shape.setOrigin(armLength * 0.5f, armThickness * 0.5f);
    shape.setFillColor(trapStats.color);
    shape.setOutlineColor(sf::Color::Black);
    shape.setOutlineThickness(2.0f);
    shape.setPosition(pivot);
}

void RotatingTrap::setArmLength(float value) {
    armLength = std::max(20.0f, value);
    stats.sizeX = armLength;
    updateShape();
}

void RotatingTrap::setArmThickness(float value) {
    armThickness = std::max(4.0f, value);
    stats.sizeY = armThickness;
    updateShape();
}

sf::FloatRect RotatingTrap::getBounds() const {
    float diameter = boundingRadius * 2.0f;
    return sf::FloatRect(pivot.x - boundingRadius, pivot.y - boundingRadius, diameter, diameter);
}

void RotatingTrap::setPosition(float x, float y) {
    pivot = sf::Vector2f(x, y);
    position = pivot;
    updateShape();
}

void RotatingTrap::setPosition(const sf::Vector2f& pos) {
    setPosition(pos.x, pos.y);
}

void RotatingTrap::update(float dt) {
    Enemy::update(dt);
    angle += rotationSpeed * dt;
    if (angle > 360.0f) angle -= 360.0f;
    if (angle < -360.0f) angle += 360.0f;
    shape.setRotation(angle);
}

void RotatingTrap::draw(sf::RenderWindow& window) {
    // Draw base (pivot)
    sf::CircleShape pivotCircle(8.0f);
    pivotCircle.setOrigin(8.0f, 8.0f);
    pivotCircle.setPosition(pivot);
    pivotCircle.setFillColor(sf::Color(100, 100, 100));
    pivotCircle.setOutlineThickness(2.0f);
    pivotCircle.setOutlineColor(sf::Color::Black);
    window.draw(pivotCircle);

    Enemy::draw(window);
}

void RotatingTrap::updateShape() {
    boundingRadius = armLength * 0.5f;
    size = sf::Vector2f(boundingRadius * 2.0f, boundingRadius * 2.0f);
    shape.setSize(sf::Vector2f(armLength, armThickness));
    shape.setOrigin(armLength * 0.5f, armThickness * 0.5f);
    shape.setPosition(pivot);
}


