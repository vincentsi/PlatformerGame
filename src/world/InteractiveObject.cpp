#include "world/InteractiveObject.h"
#include "core/Config.h"
#include <cmath>

InteractiveObject::InteractiveObject(float x, float y, float width, float height, InteractiveType type, const std::string& id)
    : position(x, y)
    , size(width, height)
    , type(type)
    , id(id)
    , activated(false)
    , interactionRange(Config::HACK_RANGE)
{
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(position);
    
    // Set color based on type
    switch (type) {
        case InteractiveType::Terminal:
            shape.setFillColor(sf::Color(50, 50, 150)); // Dark blue
            break;
        case InteractiveType::Door:
            shape.setFillColor(sf::Color(139, 69, 19)); // Brown (like platforms)
            break;
        case InteractiveType::Turret:
            shape.setFillColor(sf::Color(150, 50, 50)); // Dark red
            break;
    }
    
    shape.setOutlineColor(sf::Color::Yellow);
    shape.setOutlineThickness(2.0f);
}

void InteractiveObject::setPosition(float x, float y) {
    position.x = x;
    position.y = y;
    shape.setPosition(position);
}

void InteractiveObject::update(float dt) {
    (void)dt; // Not used for now, but kept for future animations
    // Update visual state based on activation
    if (activated) {
        // Green outline when activated
        shape.setOutlineColor(sf::Color::Green);
        
        // Slight glow effect (brighter color)
        switch (type) {
            case InteractiveType::Terminal:
                shape.setFillColor(sf::Color(80, 80, 200)); // Brighter blue
                break;
            case InteractiveType::Door:
                shape.setFillColor(sf::Color(169, 99, 49)); // Brighter brown
                break;
            case InteractiveType::Turret:
                shape.setFillColor(sf::Color(200, 80, 80)); // Brighter red
                break;
        }
    } else {
        // Yellow outline when not activated (hackable)
        shape.setOutlineColor(sf::Color::Yellow);
        
        // Reset to base color
        switch (type) {
            case InteractiveType::Terminal:
                shape.setFillColor(sf::Color(50, 50, 150));
                break;
            case InteractiveType::Door:
                shape.setFillColor(sf::Color(139, 69, 19));
                break;
            case InteractiveType::Turret:
                shape.setFillColor(sf::Color(150, 50, 50));
                break;
        }
    }
}

void InteractiveObject::draw(sf::RenderWindow& window) {
    window.draw(shape);
}

sf::FloatRect InteractiveObject::getBounds() const {
    return sf::FloatRect(position.x, position.y, size.x, size.y);
}

void InteractiveObject::activate() {
    activated = true;
}

void InteractiveObject::deactivate() {
    activated = false;
}

bool InteractiveObject::isPlayerInRange(const sf::FloatRect& playerBounds) const {
    // Calculate center of object
    sf::Vector2f center = sf::Vector2f(
        position.x + size.x / 2.0f,
        position.y + size.y / 2.0f
    );
    
    // Calculate center of player
    sf::Vector2f playerCenter = sf::Vector2f(
        playerBounds.left + playerBounds.width / 2.0f,
        playerBounds.top + playerBounds.height / 2.0f
    );
    
    // Calculate distance
    float dx = center.x - playerCenter.x;
    float dy = center.y - playerCenter.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    return distance <= interactionRange;
}

