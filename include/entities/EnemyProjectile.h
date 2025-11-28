#pragma once

#include <SFML/Graphics.hpp>

class EnemyProjectile {
public:
    EnemyProjectile(const sf::Vector2f& startPos, const sf::Vector2f& direction, float speed, float maxDistance, int damage = 1);
    ~EnemyProjectile() = default;

    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool isAlive() const { return alive; }
    void kill() { alive = false; }
    sf::Vector2f getPosition() const { return position; }
    sf::FloatRect getBounds() const;
    int getDamage() const { return damage; }

private:
    sf::Vector2f position;
    sf::Vector2f direction;
    float speed;
    float maxDistance;
    float distanceTraveled;
    bool alive;
    int damage;
    
    // Visual
    sf::CircleShape shape;
    float currentSize;
    float pulseTimer;
};

