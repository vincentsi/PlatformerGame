#include "entities/Spike.h"

Spike::Spike(float x, float y)
    : Enemy(x, y, EnemyType::Stationary, EnemyStats(1, 30.0f, 30.0f, 0.0f, 1, sf::Color(255, 100, 0)))
{
    // Make it visually distinct (red/orange color)
    shape.setOutlineColor(sf::Color::Red);
    shape.setOutlineThickness(2.0f);
}

void Spike::update(float dt) {
    if (!alive) return;
    
    // Call base class update to handle shoot timer (even though spikes don't shoot)
    Enemy::update(dt);
    
    // Stationary enemy - no movement, just exists as a hazard
    // Shape position is already set in constructor
}
