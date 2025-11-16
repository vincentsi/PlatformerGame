// Visual projectile used by Lyra's kinetic wave ability.
#include "entities/KineticWaveProjectile.h"
#include <cmath>

KineticWaveProjectile::KineticWaveProjectile(const sf::Vector2f& startPos, const sf::Vector2f& direction, float speed, float maxDistance)
    : position(startPos)
    , direction(direction)
    , speed(speed)
    , maxDistance(maxDistance)
    , distanceTraveled(0.0f)
    , alive(true)
    , currentSize(12.0f)
    , pulseTimer(0.0f)
{
    float len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len > 0.0f) {
        this->direction = sf::Vector2f(direction.x / len, direction.y / len);
    }
    
    shape.setRadius(currentSize);
    shape.setOrigin(currentSize, currentSize);
    shape.setFillColor(sf::Color(100, 200, 255, 200));
    shape.setOutlineColor(sf::Color(150, 230, 255, 255));
    shape.setOutlineThickness(2.0f);
}

void KineticWaveProjectile::update(float dt) {
    if (!alive) return;
    
    sf::Vector2f movement = direction * speed * dt;
    position += movement;
    distanceTraveled += std::sqrt(movement.x * movement.x + movement.y * movement.y);
    
    if (distanceTraveled >= maxDistance) {
        alive = false;
        return;
    }
    
    pulseTimer += dt * 8.0f;
    currentSize = 12.0f + std::sin(pulseTimer) * 3.0f;
    shape.setRadius(currentSize);
    shape.setOrigin(currentSize, currentSize);
    
    float distanceProgress = distanceTraveled / maxDistance;
    sf::Uint8 alpha = static_cast<sf::Uint8>(200 * (1.0f - distanceProgress * 0.7f));
    sf::Color currentColor = shape.getFillColor();
    currentColor.a = alpha;
    shape.setFillColor(currentColor);
    
    sf::Color outlineColor = shape.getOutlineColor();
    outlineColor.a = alpha;
    shape.setOutlineColor(outlineColor);
}

void KineticWaveProjectile::draw(sf::RenderWindow& window) {
    if (!alive) return;
    shape.setPosition(position);
    window.draw(shape);
}

