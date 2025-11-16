#pragma once

#include <SFML/Graphics.hpp>

class Entity {
public:
    Entity(float x, float y, float width, float height);
    virtual ~Entity() = default;

    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;

    // Getters
    sf::FloatRect getBounds() const;
    const sf::Vector2f& getPosition() const { return position; }
    const sf::Vector2f& getVelocity() const { return velocity; }
    const sf::Vector2f& getSize() const { return size; }

    // Setters
    void setPosition(float x, float y);
    void setPosition(const sf::Vector2f& pos);
    void setVelocity(float vx, float vy);
    void setVelocity(const sf::Vector2f& vel);

protected:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Vector2f size;
    bool isGrounded;
};
