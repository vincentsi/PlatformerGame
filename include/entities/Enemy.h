#pragma once

#include "entities/Entity.h"
#include <SFML/Graphics.hpp>

enum class EnemyType {
    Patrol,
    Stationary,
    Flying
};

// Structure pour définir les stats d'un ennemi
struct EnemyStats {
    int maxHP = 1;
    float sizeX = 30.0f;
    float sizeY = 30.0f;
    float speed = 100.0f;
    int damage = 1;  // Dégâts infligés au joueur
    sf::Color color = sf::Color::Red;
    bool canShoot = false;
    float shootCooldown = 2.0f;  // Cooldown entre les tirs
    float projectileSpeed = 300.0f;
    float projectileRange = 500.0f;  // Distance max du projectile
    float shootRange = 400.0f;  // Distance à laquelle l'ennemi commence à tirer
    
    EnemyStats() = default;
    EnemyStats(int hp, float sx, float sy, float spd, int dmg, const sf::Color& col)
        : maxHP(hp), sizeX(sx), sizeY(sy), speed(spd), damage(dmg), color(col) {}
};

class Enemy : public Entity {
public:
    Enemy(float x, float y, EnemyType type, const EnemyStats& stats = EnemyStats());
    virtual ~Enemy() = default;

    virtual void update(float dt);
    void draw(sf::RenderWindow& window) override;
    void draw(sf::RenderWindow& window, bool forceDraw); // For editor: draw even if dead

    EnemyType getType() const { return type; }
    bool isAlive() const { return alive; }
    void kill();
    void revive(); // Restore enemy to alive state (for editor)
    
    // HP system
    void takeDamage(int amount);
    int getHP() const { return currentHP; }
    int getMaxHP() const { return stats.maxHP; }
    float getHPPercent() const { return stats.maxHP > 0 ? static_cast<float>(currentHP) / static_cast<float>(stats.maxHP) : 0.0f; }
    
    // Stats
    const EnemyStats& getStats() const { return stats; }
    int getDamage() const { return stats.damage; }
    bool canShoot() const { return stats.canShoot && shootTimer <= 0.0f; }
    void resetShootTimer() { shootTimer = stats.shootCooldown; }

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
    EnemyStats stats;
    int currentHP;
    float shootTimer;  // Timer pour le cooldown de tir

    // Patrol boundaries
    float patrolLeftBound;
    float patrolRightBound;
};
