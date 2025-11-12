#include "entities/Entity.h"

Entity::Entity(float x, float y, float width, float height)
    : position(x, y)
    , velocity(0.0f, 0.0f)
    , size(width, height)
    , isGrounded(false)
{
}

sf::FloatRect Entity::getBounds() const {
    return sf::FloatRect(position.x, position.y, size.x, size.y);
}

void Entity::setPosition(float x, float y) {
    position.x = x;
    position.y = y;
}

void Entity::setPosition(const sf::Vector2f& pos) {
    position = pos;
}

void Entity::setVelocity(float vx, float vy) {
    velocity.x = vx;
    velocity.y = vy;
}

void Entity::setVelocity(const sf::Vector2f& vel) {
    velocity = vel;
}
