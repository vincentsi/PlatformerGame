#pragma once

#include "entities/Entity.h"
#include <SFML/Graphics.hpp>

enum class EnemyType {
    Patrol,
    Stationary,
    Flying
};

class Enemy : public Entity {
public:
    Enemy(float x, float y, EnemyType type);
    virtual ~Enemy() = default;

    virtual void update(float dt) = 0;
    void draw(sf::RenderWindow& window) override;

    EnemyType getType() const { return type; }
    bool isAlive() const { return alive; }
    void kill();

    // Movement boundaries for patrol enemies
    void setPatrolBounds(float leftBound, float rightBound);
    float getLeftBound() const { return patrolLeftBound; }
    float getRightBound() const { return patrolRightBound; }
    
    // Get/set patrol distance (distance from center)
    float getPatrolDistance() const;
    void setPatrolDistance(float distance);
    
    // Override setPosition to update shape
    void setPosition(float x, float y) override;
    void setPosition(const sf::Vector2f& pos) override;

protected:
    sf::RectangleShape shape;
    EnemyType type;
    bool alive;
    float speed;

    // Patrol boundaries
    float patrolLeftBound;
    float patrolRightBound;
};
