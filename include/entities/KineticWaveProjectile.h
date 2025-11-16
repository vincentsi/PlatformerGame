#pragma once

#include <SFML/Graphics.hpp>

class KineticWaveProjectile {
public:
    KineticWaveProjectile(const sf::Vector2f& startPos, const sf::Vector2f& direction, float speed, float maxDistance);
    ~KineticWaveProjectile() = default;

    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool isAlive() const { return alive; }
    sf::Vector2f getPosition() const { return position; }
    float getDistanceTraveled() const { return distanceTraveled; }

private:
    sf::Vector2f position;
    sf::Vector2f direction;
    float speed;
    float maxDistance;
    float distanceTraveled;
    bool alive;
    
    // Visual
    sf::CircleShape shape;
    float currentSize;
    float pulseTimer;
};

