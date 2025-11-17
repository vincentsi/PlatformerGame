#include "world/GoalZone.h"
#include <cmath>

GoalZone::GoalZone(float x, float y, float width, float height)
    : Entity(x, y, width, height)
    , animationTime(0.0f)
{
    shape.setSize(sf::Vector2f(size.x, size.y));
    shape.setPosition(position);
    shape.setFillColor(sf::Color(255, 215, 0, 180)); // Gold color, semi-transparent
    shape.setOutlineColor(sf::Color::Yellow);
    shape.setOutlineThickness(3.0f);
}

void GoalZone::update(float dt) {
    // Animate the goal zone (pulsing effect)
    animationTime += dt * 2.0f; // Speed of animation

    // Pulsing alpha between 100 and 220
    int alpha = static_cast<int>(160 + 60 * std::sin(animationTime));
    shape.setFillColor(sf::Color(255, 215, 0, static_cast<sf::Uint8>(alpha)));
}

void GoalZone::draw(sf::RenderWindow& window) {
    window.draw(shape);
}

bool GoalZone::isPlayerInside(const sf::FloatRect& playerBounds) const {
    return getBounds().intersects(playerBounds);
}
