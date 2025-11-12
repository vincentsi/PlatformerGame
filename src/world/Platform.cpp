#include "world/Platform.h"

Platform::Platform(float x, float y, float width, float height)
    : position(x, y)
    , size(width, height)
{
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(position);
    shape.setFillColor(sf::Color(139, 69, 19)); // Brown
    shape.setOutlineColor(sf::Color::Black);
    shape.setOutlineThickness(2.0f);
}

void Platform::draw(sf::RenderWindow& window) {
    window.draw(shape);
}

sf::FloatRect Platform::getBounds() const {
    return sf::FloatRect(position.x, position.y, size.x, size.y);
}
