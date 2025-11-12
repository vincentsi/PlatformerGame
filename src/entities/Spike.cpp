#include "entities/Spike.h"

Spike::Spike(float x, float y)
    : Enemy(x, y, EnemyType::Stationary)
{
    speed = 0.0f; // Stationary, no movement

    // Make it visually distinct (red/orange color, smaller size)
    shape.setSize(sf::Vector2f(30.0f, 30.0f));
    shape.setFillColor(sf::Color(255, 100, 0)); // Orange
    shape.setOutlineColor(sf::Color::Red);
    shape.setOutlineThickness(2.0f);

    // Update size in Entity
    size = sf::Vector2f(30.0f, 30.0f);
}

void Spike::update(float dt) {
    // Stationary enemy - no movement, just exists as a hazard
    // Shape position is already set in constructor
    // Nothing to update
}
