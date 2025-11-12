#include "entities/Player.h"
#include "core/Config.h"
#include "core/InputConfig.h"
#include "physics/PhysicsConstants.h"
#include <cmath>

Player::Player(float x, float y, CharacterType type)
    : Entity(x, y, Config::PLAYER_WIDTH, Config::PLAYER_HEIGHT)
    , characterType(type)
    , coyoteTimeCounter(0.0f)
    , jumpBufferCounter(0.0f)
    , jumpPressed(false)
    , isJumping(false)
    , jumpReleased(true)
    , jumpsRemaining(1)
    , health(3)
    , maxHealth(3)
    , invincibleTimer(0.0f)
    , invincibleDuration(1.5f)
    , dead(false)
    , spawnPoint(x, y)
    , respawnTimer(0.0f)
    , justJumped(false)
    , justLanded(false)
    , wasGrounded(false)
{
    shape.setSize(sf::Vector2f(size.x, size.y));

    // Set color based on character type
    switch (characterType) {
        case CharacterType::Lyra:
            shape.setFillColor(sf::Color::Green); // Lyra = Green
            break;
        case CharacterType::Noah:
            shape.setFillColor(sf::Color::Blue); // Noah = Blue
            break;
        case CharacterType::Sera:
            shape.setFillColor(sf::Color::Magenta); // Sera = Magenta
            break;
    }

    shape.setOutlineColor(sf::Color::Black);
    shape.setOutlineThickness(2.0f);
}

void Player::update(float dt) {
    // Handle death state
    if (dead) {
        respawnTimer -= dt;
        if (respawnTimer <= 0.0f) {
            respawn();
        }
        return; // Don't update movement while dead
    }

    // Update invincibility timer
    if (invincibleTimer > 0.0f) {
        invincibleTimer -= dt;
    }

    // Check death zone (fell off map)
    if (position.y > Config::DEATH_ZONE_Y) {
        die();
        return;
    }

    // Apply gravity
    applyGravity(dt);

    // Update timers
    updateCoyoteTime(dt);
    updateJumpBuffer(dt);

    // Apply velocity to position
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    // Update shape position
    shape.setPosition(position);

    // Variable jump height: if player releases jump button early, cut upward velocity
    if (isJumping && jumpReleased && velocity.y < 0.0f) {
        velocity.y *= 0.5f;
        isJumping = false;
    }

    // Reset jump released flag (check if jump key is released)
    // Note: This uses InputConfig to respect user key bindings
    const InputBindings& bindings = InputConfig::getInstance().getBindings();
    if (!sf::Keyboard::isKeyPressed(bindings.jump)) {
        jumpReleased = true;
    }
}

void Player::draw(sf::RenderWindow& window) {
    // Flicker effect when invincible (blink every 0.1 seconds)
    if (invincibleTimer > 0.0f) {
        int flicker = static_cast<int>(invincibleTimer * 10) % 2;
        if (flicker == 0) {
            return; // Skip drawing (creates blink effect)
        }
    }
    window.draw(shape);
}

void Player::moveLeft() {
    velocity.x = -getMoveSpeed();
}

void Player::moveRight() {
    velocity.x = getMoveSpeed();
}

void Player::jump() {
    // Ground jump or coyote time jump
    if ((isGrounded || coyoteTimeCounter > 0.0f) && jumpReleased) {
        velocity.y = Config::JUMP_VELOCITY;
        isGrounded = false;
        coyoteTimeCounter = 0.0f;
        isJumping = true;
        jumpReleased = false;
        justJumped = true;
        jumpsRemaining = getMaxJumps() - 1;  // Reset air jumps
    }
    // Air jump (double jump for Lyra)
    else if (!isGrounded && jumpsRemaining > 0 && jumpReleased && canDoubleJump()) {
        velocity.y = Config::JUMP_VELOCITY * 0.9f;  // Slightly weaker air jump
        isJumping = true;
        jumpReleased = false;
        justJumped = true;
        jumpsRemaining--;
    }
    // Store jump input for jump buffering
    else if (!isGrounded && !jumpReleased) {
        jumpBufferCounter = Config::JUMP_BUFFER;
    }
}

void Player::stopMoving() {
    // Apply friction
    velocity.x *= Config::FRICTION;

    // Stop completely if very slow
    if (std::abs(velocity.x) < 1.0f) {
        velocity.x = 0.0f;
    }
}

void Player::setGrounded(bool grounded) {
    // Just landed (transition from air to ground)
    if (grounded && !wasGrounded) {
        isJumping = false;
        jumpsRemaining = getMaxJumps();  // Reset jumps on landing

        // Only trigger landing effects if we're not immediately jump buffering
        bool willJumpBuffer = (jumpBufferCounter > 0.0f);

        if (!willJumpBuffer) {
            justLanded = true;  // Set event flag for effects
        }

        // Jump buffer: if jump was pressed recently, execute it now
        if (jumpBufferCounter > 0.0f) {
            velocity.y = Config::JUMP_VELOCITY;
            grounded = false;  // We're jumping, so not grounded anymore
            jumpBufferCounter = 0.0f;
            isJumping = true;
            justJumped = true;
            jumpsRemaining = getMaxJumps() - 1;  // Used one jump
        }
    }

    // Just left ground (fell off platform)
    if (!grounded && wasGrounded) {
        coyoteTimeCounter = Config::COYOTE_TIME;
    }

    // Update state
    wasGrounded = isGrounded;
    isGrounded = grounded;
}

void Player::applyGravity(float dt) {
    if (!isGrounded) {
        velocity.y += Physics::GRAVITY * dt;

        // Terminal velocity
        if (velocity.y > Physics::TERMINAL_VELOCITY) {
            velocity.y = Physics::TERMINAL_VELOCITY;
        }
    }
}

void Player::updateCoyoteTime(float dt) {
    if (coyoteTimeCounter > 0.0f) {
        coyoteTimeCounter -= dt;
    }
}

void Player::updateJumpBuffer(float dt) {
    if (jumpBufferCounter > 0.0f) {
        jumpBufferCounter -= dt;
    }
}

void Player::takeDamage(int amount) {
    // Check if invincible
    if (invincibleTimer > 0.0f) {
        return; // No damage during invincibility
    }

    // Reduce health
    health -= amount;
    if (health < 0) health = 0;

    // Activate invincibility
    invincibleTimer = invincibleDuration;

    // Die if health reaches 0
    if (health <= 0) {
        die();
    }

    // TODO: Play damage sound, trigger damage particles
}

void Player::heal(int amount) {
    health += amount;
    if (health > maxHealth) {
        health = maxHealth;
    }
}

void Player::die() {
    if (!dead) {
        dead = true;
        respawnTimer = Config::RESPAWN_TIME;
        velocity = sf::Vector2f(0.0f, 0.0f);
        shape.setFillColor(sf::Color::Red); // Visual feedback of death
    }
}

void Player::respawn() {
    dead = false;
    position = spawnPoint;
    velocity = sf::Vector2f(0.0f, 0.0f);
    isGrounded = false;
    coyoteTimeCounter = 0.0f;
    jumpBufferCounter = 0.0f;
    isJumping = false;
    jumpReleased = true;
    health = maxHealth; // Reset health on respawn
    invincibleTimer = 0.0f; // Reset invincibility
    shape.setPosition(position);
    
    // Reset color based on character type
    switch (characterType) {
        case CharacterType::Lyra:
            shape.setFillColor(sf::Color::Green);
            break;
        case CharacterType::Noah:
            shape.setFillColor(sf::Color::Blue);
            break;
        case CharacterType::Sera:
            shape.setFillColor(sf::Color::Magenta);
            break;
    }
}

void Player::setSpawnPoint(float x, float y) {
    spawnPoint = sf::Vector2f(x, y);
}

void Player::clearEventFlags() {
    justJumped = false;
    justLanded = false;
}

// Character abilities based on type
bool Player::canDoubleJump() const {
    return characterType == CharacterType::Lyra;  // Only Lyra can double jump
}

int Player::getMaxJumps() const {
    switch (characterType) {
        case CharacterType::Lyra:
            return 2;  // Lyra has double jump
        case CharacterType::Noah:
        case CharacterType::Sera:
        default:
            return 1;  // Noah and Sera have single jump
    }
}

float Player::getMoveSpeed() const {
    switch (characterType) {
        case CharacterType::Lyra:
            return Config::MOVE_SPEED;  // Lyra = normal speed (300)
        case CharacterType::Noah:
            return Config::MOVE_SPEED * 0.75f;  // Noah = slower (225)
        case CharacterType::Sera:
            return Config::MOVE_SPEED * 1.15f;  // Sera = faster (345)
        default:
            return Config::MOVE_SPEED;
    }
}

float Player::getStompDamageMultiplier() const {
    switch (characterType) {
        case CharacterType::Noah:
            return 1.5f;  // Noah has stronger stomp (can kill from higher)
        case CharacterType::Lyra:
        case CharacterType::Sera:
        default:
            return 1.0f;  // Normal stomp
    }
}
