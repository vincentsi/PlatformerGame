#include "entities/EnemyProjectile.h"
#include <cmath>

EnemyProjectile::EnemyProjectile(const sf::Vector2f& startPos, const sf::Vector2f& direction, float speed, float maxDistance, int damage)
    : position(startPos)
    , direction(direction)
    , speed(speed)
    , maxDistance(maxDistance)
    , distanceTraveled(0.0f)
    , alive(true)
    , damage(damage)
    , currentSize(8.0f)
    , pulseTimer(0.0f)
{
    float len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len > 0.0f) {
        this->direction = sf::Vector2f(direction.x / len, direction.y / len);
    }
    
    shape.setRadius(currentSize);
    shape.setOrigin(currentSize, currentSize);
    shape.setFillColor(sf::Color(255, 100, 100, 220)); // Reddish projectile
    shape.setOutlineColor(sf::Color(255, 150, 150, 255));
    shape.setOutlineThickness(1.5f);
}

void EnemyProjectile::update(float dt) {
    if (!alive) return;
    
    sf::Vector2f movement = direction * speed * dt;
    position += movement;
    distanceTraveled += std::sqrt(movement.x * movement.x + movement.y * movement.y);
    
    if (distanceTraveled >= maxDistance) {
        alive = false;
        return;
    }
    
    pulseTimer += dt * 10.0f;
    currentSize = 8.0f + std::sin(pulseTimer) * 2.0f;
    shape.setRadius(currentSize);
    shape.setOrigin(currentSize, currentSize);
}

void EnemyProjectile::draw(sf::RenderWindow& window) {
    if (!alive) return;
    shape.setPosition(position);
    window.draw(shape);
}

sf::FloatRect EnemyProjectile::getBounds() const {
    return sf::FloatRect(
        position.x - currentSize,
        position.y - currentSize,
        currentSize * 2.0f,
        currentSize * 2.0f
    );
}

