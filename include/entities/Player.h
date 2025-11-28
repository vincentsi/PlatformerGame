#pragma once

#include "Entity.h"
#include <SFML/Graphics.hpp>
#include "graphics/SpriteManager.h"
#include <vector>
#include <string>

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
    void attack();
    bool canAttack() const;
    void dash();
    bool canDash() const;

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

    // Special abilities
    void useAbility();
    bool canUseAbility() const;
    float getAbilityCooldown() const;
    float getAbilityCooldownRemaining() const;
    float getAttackCooldownRemaining() const { return attackCooldownRemaining; }
    float getDashCooldownRemaining() const { return dashCooldownRemaining; }
    int getFacingDirection() const { return facingDirection; } // 1 = right, -1 = left, 0 = down, 2 = up
    
    // Ability states
    bool isBerserkMode() const { return berserkActive; }
    bool isHacking() const { return hacking; }
    sf::Vector2f getKineticWaveDirection() const { return kineticWaveDirection; }
    bool hasKineticWaveActive() const { return kineticWaveActive; }
    bool hasKineticWaveJustActivated() const { return kineticWaveJustActivated; }
    void clearKineticWaveActivation() { kineticWaveJustActivated = false; }
    float getAbilityAnimationTimer() const { return abilityAnimationTimer; }

private:
    void applyGravity(float dt);
    void updateCoyoteTime(float dt);
    void updateJumpBuffer(float dt);
    void updateAbilityCooldown(float dt);
    
    // Special ability implementations
    void useKineticWave();      // Lyra
    void useHack();              // Noah
    void useBerserk();           // Sera

private:
    sf::RectangleShape shape;  // Fallback si pas de sprite
    sf::Sprite sprite;         // Sprite actuel
    std::vector<sf::Texture*> idleTexturesSouth;  // Textures pour l'animation idle (face caméra/bas)
    std::vector<sf::Texture*> idleTexturesNorth;  // Textures pour dos/haut
    std::vector<sf::Texture*> idleTexturesEast;   // Textures pour droite
    std::vector<sf::Texture*> idleTexturesWest;   // Textures pour gauche
    std::vector<sf::Texture*> runTexturesSouth;   // Textures pour l'animation run (face caméra/bas)
    std::vector<sf::Texture*> runTexturesNorth;   // Textures pour run dos/haut
    std::vector<sf::Texture*> runTexturesEast;    // Textures pour run droite
    std::vector<sf::Texture*> runTexturesWest;    // Textures pour run gauche
    std::vector<sf::Texture*> jumpTexturesSouth;  // Textures pour l'animation jump
    std::vector<sf::Texture*> jumpTexturesNorth;
    std::vector<sf::Texture*> jumpTexturesEast;
    std::vector<sf::Texture*> jumpTexturesWest;
    std::vector<sf::Texture*> doubleJumpTexturesSouth; // Textures pour double jump
    std::vector<sf::Texture*> doubleJumpTexturesNorth;
    std::vector<sf::Texture*> doubleJumpTexturesEast;
    std::vector<sf::Texture*> doubleJumpTexturesWest;
    std::vector<sf::Texture*> hurtTexturesSouth;  // Textures pour l'animation hurt
    std::vector<sf::Texture*> hurtTexturesNorth;
    std::vector<sf::Texture*> hurtTexturesEast;
    std::vector<sf::Texture*> hurtTexturesWest;
    std::vector<sf::Texture*> deathTexturesSouth; // Textures pour l'animation death
    std::vector<sf::Texture*> deathTexturesNorth;
    std::vector<sf::Texture*> deathTexturesEast;
    std::vector<sf::Texture*> deathTexturesWest;
    std::vector<sf::Texture*> abilityTexturesSouth; // Textures pour l'animation ability (Kinetic Wave)
    std::vector<sf::Texture*> abilityTexturesNorth;
    std::vector<sf::Texture*> abilityTexturesEast;
    std::vector<sf::Texture*> abilityTexturesWest;
    std::vector<sf::Texture*> kickTexturesSouth; // Textures pour l'animation kick/attack
    std::vector<sf::Texture*> kickTexturesNorth;
    std::vector<sf::Texture*> kickTexturesEast;
    std::vector<sf::Texture*> kickTexturesWest;
    std::vector<sf::Texture*> currentAnimationTextures; // Textures de l'animation actuelle
    int currentAnimationFrame;
    float animationTimer;
    float idleFrameDuration;
    float runFrameDuration;
    float jumpFrameDuration;
    float doubleJumpFrameDuration;
    float hurtFrameDuration;
    float deathFrameDuration;
    float abilityFrameDuration;
    float kickFrameDuration;
    bool useSprites;  // Utilise les sprites ou le rectangle coloré?
    int facingDirection; // 0 = face (south/bas), 2 = dos (north/haut), 1 = droite (east), -1 = gauche (west)
    bool isRunning;   // true = run animation, false = idle animation
    float hurtAnimationTimer; // Timer pour animation hurt (joue une fois)
    float abilityAnimationTimer; // Timer pour animation ability (joue une fois)
    float attackAnimationTimer; // Timer pour animation attack/kick (joue une fois)
    
    enum class AnimationState {
        Idle,
        Run,
        Jump,
        DoubleJump,
        Hurt,
        Death,
        Ability,
        Attack
    };
    AnimationState currentAnimationState;
    
    CharacterType characterType;
    
    void loadIdleAnimation();
    void loadRunAnimation();
    void loadJumpAnimation();
    void loadDoubleJumpAnimation();
    void loadHurtAnimation();
    void loadDeathAnimation();
    void loadAbilityAnimation();
    void loadKickAnimation();
    void updateAnimation(float dt);

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

    // Special abilities
    float abilityCooldown;
    float abilityCooldownRemaining;
    float attackCooldownRemaining;
    
    // Dash
    bool dashing;
    float dashTimer;
    float dashCooldownRemaining;
    
    // Lyra - Kinetic Wave
    bool kineticWaveActive;
    bool kineticWaveJustActivated;  // True for one frame when activated
    float kineticWaveTimer;
    sf::Vector2f kineticWaveDirection;
    
    // Noah - Hack
    bool hacking;
    float hackTimer;
    
    // Sera - Berserk Mode
    bool berserkActive;
    float berserkTimer;
    float berserkHealAccumulator;
};
