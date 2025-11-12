#pragma once

#include "Entity.h"
#include <SFML/Graphics.hpp>

enum class CharacterType {
    Lyra,
    Noah,
    Sera
};

class Player : public Entity {
public:
    Player(float x, float y, CharacterType type = CharacterType::Lyra);
    ~Player() override = default;

    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

    void moveLeft();
    void moveRight();
    void jump();
    void stopMoving();

    void setGrounded(bool grounded);
    bool getIsGrounded() const { return isGrounded; }

    // Health system
    void takeDamage(int amount = 1);
    void heal(int amount = 1);
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    bool isInvincible() const { return invincibleTimer > 0.0f; }

    // Death and respawn
    void die();
    void respawn();
    bool isDead() const { return dead; }
    void setSpawnPoint(float x, float y);
    sf::Vector2f getSpawnPoint() const { return spawnPoint; }

    // Event tracking for effects
    bool hasJustJumped() const { return justJumped; }
    bool hasJustLanded() const { return justLanded; }
    void clearEventFlags();

    // Character type
    CharacterType getCharacterType() const { return characterType; }
    std::string getCharacterName() const;

    // Character abilities
    bool canDoubleJump() const;
    int getMaxJumps() const;
    float getMoveSpeed() const;
    float getStompDamageMultiplier() const;

private:
    void applyGravity(float dt);
    void updateCoyoteTime(float dt);
    void updateJumpBuffer(float dt);

private:
    sf::RectangleShape shape;
    CharacterType characterType;

    // Coyote time (grace period for jumping after leaving platform)
    float coyoteTimeCounter;

    // Jump buffering (allow jump input slightly before landing)
    float jumpBufferCounter;
    bool jumpPressed;

    // Variable jump height
    bool isJumping;
    bool jumpReleased;
    int jumpsRemaining;  // For double jump mechanic

    // Health system
    int health;
    int maxHealth;
    float invincibleTimer;
    float invincibleDuration;

    // Death and respawn
    bool dead;
    sf::Vector2f spawnPoint;
    float respawnTimer;

    // Event flags for effects
    bool justJumped;
    bool justLanded;
    bool wasGrounded;
};
