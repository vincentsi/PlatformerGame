#include "world/Checkpoint.h"
#include <cmath>

Checkpoint::Checkpoint(float x, float y, const std::string& id)
    : position(x, y)
    , id(id)
    , activated(false)
    , pulseTimer(0.0f)
    , inactiveColor(100, 100, 100, 150)   // Gray transparent
    , activeColor(100, 255, 100, 200)      // Green transparent
{
    shape.setSize(sf::Vector2f(40.0f, 60.0f));
    shape.setPosition(position);
    shape.setFillColor(inactiveColor);
    shape.setOutlineThickness(2.0f);
    shape.setOutlineColor(sf::Color(200, 200, 200));
}

void Checkpoint::update(float dt) {
    if (activated) {
        // Pulsing animation when active
        pulseTimer += dt * 3.0f;
        float pulse = (std::sin(pulseTimer) + 1.0f) / 2.0f; // 0 to 1

        sf::Color currentColor = activeColor;
        currentColor.a = static_cast<sf::Uint8>(150 + pulse * 50); // Pulse between 150-200 alpha
        shape.setFillColor(currentColor);
        shape.setOutlineColor(sf::Color(100, 255, 100));
    }
}

void Checkpoint::draw(sf::RenderWindow& window) {
    window.draw(shape);
}

bool Checkpoint::isPlayerInside(const sf::FloatRect& playerBounds) const {
    return shape.getGlobalBounds().intersects(playerBounds);
}

void Checkpoint::activate() {
    if (!activated) {
        activated = true;
        shape.setFillColor(activeColor);
        pulseTimer = 0.0f;
    }
}

sf::Vector2f Checkpoint::getSpawnPosition() const {
    // Spawn slightly to the left of checkpoint
    return sf::Vector2f(position.x - 20.0f, position.y);
}
