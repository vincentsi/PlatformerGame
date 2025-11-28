// Player entity: movement, jump logic, abilities and animation
// for the three playable characters (Lyra / Noah / Sera).
#include "entities/Player.h"
#include "core/Config.h"
#include "core/InputConfig.h"
#include "physics/PhysicsConstants.h"
#include <cmath>

Player::Player(float x, float y, CharacterType type)
    : Entity(x, y, 
             (type == CharacterType::Lyra) ? Config::PLAYER_WIDTH - 4.0f : 
             (type == CharacterType::Noah) ? Config::PLAYER_WIDTH - 8.0f : Config::PLAYER_WIDTH,
             (type == CharacterType::Lyra) ? Config::PLAYER_HEIGHT + 15.0f : Config::PLAYER_HEIGHT)
    , characterType(type)
    , currentAnimationFrame(0)
    , animationTimer(0.0f)
    , idleFrameDuration(0.133f)      // ~7.5 FPS (4 frames)
    , runFrameDuration(0.083f)       // ~12 FPS (4 frames)
    , jumpFrameDuration(0.1f)
    , doubleJumpFrameDuration(0.1f)
    , hurtFrameDuration(0.1f)
    , deathFrameDuration(0.15f)      // un peu plus lent pour bien lire l'anim
    , abilityFrameDuration(0.1f)
    , kickFrameDuration(0.08f)      // Kick animation frames (rapide)
    , useSprites(false)
    , facingDirection(0)  // 0 = face caméra, 1 = droite, -1 = gauche
    , isRunning(false)
    , hurtAnimationTimer(0.0f)
    , abilityAnimationTimer(0.0f)
    , attackAnimationTimer(0.0f)
    , currentAnimationState(AnimationState::Idle)
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
    , abilityCooldown(0.0f)
    , abilityCooldownRemaining(0.0f)
    , attackCooldownRemaining(0.0f)
    , dashing(false)
    , dashTimer(0.0f)
    , dashCooldownRemaining(0.0f)
    , kineticWaveActive(false)
    , kineticWaveJustActivated(false)
    , kineticWaveTimer(0.0f)
    , kineticWaveDirection(0.0f, 0.0f)
    , hacking(false)
    , hackTimer(0.0f)
    , berserkActive(false)
    , berserkTimer(0.0f)
    , berserkHealAccumulator(0.0f)
{
    shape.setSize(sf::Vector2f(size.x, size.y));
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

    shape.setOutlineColor(sf::Color::Black);
    shape.setOutlineThickness(2.0f);
    
    loadIdleAnimation();
    loadRunAnimation();
    loadJumpAnimation();
    loadDoubleJumpAnimation();
    loadHurtAnimation();
    loadDeathAnimation();
    loadAbilityAnimation();
    loadKickAnimation();
    
    sprite.setPosition(position);
}

void Player::update(float dt) {
    if (useSprites) {
        updateAnimation(dt);
    }
    
    if (hurtAnimationTimer > 0.0f) {
        hurtAnimationTimer -= dt;
    }
    
    if (abilityAnimationTimer > 0.0f) {
        abilityAnimationTimer -= dt;
    }
    
    if (attackAnimationTimer > 0.0f) {
        attackAnimationTimer -= dt;
    }
    
    if (dead) {
        respawnTimer -= dt;
        if (respawnTimer <= 0.0f) {
            respawn();
        }
        return;
    }

    if (invincibleTimer > 0.0f) {
        invincibleTimer -= dt;
    }

    if (position.y > Config::DEATH_ZONE_Y) {
        die();
        return;
    }

    applyGravity(dt);

    updateCoyoteTime(dt);
    updateJumpBuffer(dt);
    updateAbilityCooldown(dt);
    
    // Update dash
    if (dashing) {
        dashTimer -= dt;
        if (dashTimer <= 0.0f) {
            dashing = false;
        }
    }
    
    if (dashCooldownRemaining > 0.0f) {
        dashCooldownRemaining -= dt;
        if (dashCooldownRemaining < 0.0f) {
            dashCooldownRemaining = 0.0f;
        }
    }
    
    if (kineticWaveActive) {
        kineticWaveTimer -= dt;
        if (kineticWaveTimer <= 0.0f) {
            kineticWaveActive = false;
            kineticWaveJustActivated = false;
        }
    }
    
    if (hacking) {
        hackTimer -= dt;
        if (hackTimer <= 0.0f) {
            hacking = false;
        }
    }
    
    if (berserkActive) {
        berserkTimer -= dt;
        
        berserkHealAccumulator += Config::BERSERK_HEAL_RATE * dt;
        if (berserkHealAccumulator >= 1.0f) {
            int healAmount = static_cast<int>(berserkHealAccumulator);
            heal(healAmount);
            berserkHealAccumulator -= static_cast<float>(healAmount);
        }
        
        // Visual feedback - pulse effect
        float pulse = std::sin(berserkTimer * 10.0f) * 0.5f + 0.5f;
        sf::Color currentColor = shape.getFillColor();
        if (characterType == CharacterType::Sera) {
            shape.setFillColor(sf::Color(255, static_cast<sf::Uint8>(100 + pulse * 100), 255));
        }
        
        if (berserkTimer <= 0.0f) {
            berserkActive = false;
            switch (characterType) {
                case CharacterType::Sera:
                    shape.setFillColor(sf::Color::Magenta);
                    break;
                default:
                    break;
            }
        }
    }

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    shape.setPosition(position);
    
    if (useSprites) {
        // Sprite origin = pieds; position du sprite = bas-centre du hitbox.
        float offsetY = 0.0f;
        sprite.setPosition(position.x + size.x * 0.5f, position.y + size.y + offsetY);
    }

    if (isJumping && jumpReleased && velocity.y < 0.0f) {
        velocity.y *= 0.5f;
        isJumping = false;
    }

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
    
    // Draw sprite if available, otherwise fallback to rectangle
    if (useSprites && !currentAnimationTextures.empty()) {
        window.draw(sprite);
    } else {
        window.draw(shape);
    }
}

void Player::moveLeft() {
    if (!dashing) {
        velocity.x = -getMoveSpeed();
    }
}

void Player::moveRight() {
    if (!dashing) {
        velocity.x = getMoveSpeed();
    }
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
    // Don't apply friction during dash
    if (dashing) {
        return;
    }
    
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
    
    // Trigger hurt animation (plays once during invincibility period)
    hurtAnimationTimer = 0.3f; // Play hurt animation for 0.3 seconds

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
        
        // Reset animation state to trigger death animation
        currentAnimationState = AnimationState::Death;
        currentAnimationFrame = 0;
        animationTimer = 0.0f;
        
        // Load death textures based on current facing direction (for all characters)
        if (useSprites) {
            const std::vector<sf::Texture*>* deathTextures = nullptr;
            
            if (facingDirection == 2 && !deathTexturesNorth.empty()) {
                deathTextures = &deathTexturesNorth;
            } else if (facingDirection == 1 && !deathTexturesEast.empty()) {
                deathTextures = &deathTexturesEast;
            } else if (facingDirection == -1 && !deathTexturesWest.empty()) {
                deathTextures = &deathTexturesWest;
            } else if (!deathTexturesSouth.empty()) {
                deathTextures = &deathTexturesSouth;
            }
            
            if (deathTextures && !deathTextures->empty()) {
                currentAnimationTextures = *deathTextures;
                sprite.setTexture(*currentAnimationTextures[0]);
            }
        }
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
    hurtAnimationTimer = 0.0f; // Reset hurt animation
    currentAnimationState = AnimationState::Idle; // Reset animation state
    currentAnimationFrame = 0; // Reset animation frame
    animationTimer = 0.0f; // Reset animation timer
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
    
    // Reset sprite to idle if using sprites
    if (useSprites && !idleTexturesSouth.empty()) {
        currentAnimationTextures = idleTexturesSouth;
        sprite.setTexture(*currentAnimationTextures[0]);
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
    float baseSpeed;
    switch (characterType) {
        case CharacterType::Lyra:
            baseSpeed = Config::MOVE_SPEED;  // Lyra = normal speed (300)
            break;
        case CharacterType::Noah:
            baseSpeed = Config::MOVE_SPEED * 0.75f;  // Noah = slower (225)
            break;
        case CharacterType::Sera:
            baseSpeed = Config::MOVE_SPEED * 1.15f;  // Sera = faster (345)
            break;
        default:
            baseSpeed = Config::MOVE_SPEED;
            break;
    }
    
    // Apply berserk speed boost
    if (berserkActive && characterType == CharacterType::Sera) {
        baseSpeed *= Config::BERSERK_SPEED_BOOST;
    }
    
    return baseSpeed;
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

bool Player::canAttack() const {
    return attackCooldownRemaining <= 0.0f && !dead;
}

void Player::attack() {
    if (!canAttack()) {
        return;
    }
    
    // Set attack cooldown
    attackCooldownRemaining = Config::ATTACK_COOLDOWN;
    
    // Trigger attack animation (duration based on number of frames)
    // high-kick animation has 7 frames
    attackAnimationTimer = 0.4f; // ~0.4 seconds for kick animation
    
    // Attack logic will be handled in Game::update() to check for enemy collisions
    // This just triggers the attack state
}

bool Player::canDash() const {
    return dashCooldownRemaining <= 0.0f && !dead && !dashing;
}

void Player::dash() {
    if (!canDash()) {
        return;
    }
    
    // Determine dash direction based on facing direction or velocity
    float dashDirection = 0.0f;
    if (std::abs(velocity.x) > 0.1f) {
        dashDirection = (velocity.x > 0.0f) ? 1.0f : -1.0f;
    } else if (facingDirection == 1) {
        dashDirection = 1.0f; // Right
    } else if (facingDirection == -1) {
        dashDirection = -1.0f; // Left
    } else {
        dashDirection = 1.0f; // Default to right if no direction
    }
    
    // Set dash velocity
    dashing = true;
    dashTimer = Config::DASH_DURATION;
    velocity.x = dashDirection * Config::DASH_SPEED;
    
    // Set cooldown
    dashCooldownRemaining = Config::DASH_COOLDOWN;
}

// Special abilities implementation
void Player::useAbility() {
    if (!canUseAbility()) {
        return;
    }
    
    switch (characterType) {
        case CharacterType::Lyra:
            useKineticWave();
            break;
        case CharacterType::Noah:
            useHack();
            break;
        case CharacterType::Sera:
            useBerserk();
            break;
        default:
            break;
    }
}

bool Player::canUseAbility() const {
    return abilityCooldownRemaining <= 0.0f && !dead;
}

float Player::getAbilityCooldown() const {
    switch (characterType) {
        case CharacterType::Lyra:
            return Config::KINETIC_WAVE_COOLDOWN;
        case CharacterType::Noah:
            return Config::HACK_COOLDOWN;
        case CharacterType::Sera:
            return Config::BERSERK_COOLDOWN;
        default:
            return 0.0f;
    }
}

float Player::getAbilityCooldownRemaining() const {
    return abilityCooldownRemaining;
}

void Player::updateAbilityCooldown(float dt) {
    if (abilityCooldownRemaining > 0.0f) {
        abilityCooldownRemaining -= dt;
        if (abilityCooldownRemaining < 0.0f) {
            abilityCooldownRemaining = 0.0f;
        }
    }
    
    // Update attack cooldown
    if (attackCooldownRemaining > 0.0f) {
        attackCooldownRemaining -= dt;
        if (attackCooldownRemaining < 0.0f) {
            attackCooldownRemaining = 0.0f;
        }
    }
    
    // Update dash cooldown (already handled in update(), but keeping for consistency)
}

// Lyra - Kinetic Wave
void Player::useKineticWave() {
    // Determine direction based on player facing
    float directionX = (velocity.x > 0.0f) ? 1.0f : (velocity.x < 0.0f) ? -1.0f : 1.0f;
    kineticWaveDirection = sf::Vector2f(directionX, 0.0f);
    
    kineticWaveActive = true;
    kineticWaveJustActivated = true;  // Mark as just activated
    kineticWaveTimer = 0.2f;  // Visual effect duration
    
    // Trigger ability animation (plays once)
    abilityAnimationTimer = 0.6f; // Play ability animation for ~0.6 seconds (6 frames * 0.1s)
    
    // Set cooldown
    abilityCooldown = Config::KINETIC_WAVE_COOLDOWN;
    abilityCooldownRemaining = abilityCooldown;
}

// Noah - Hack
void Player::useHack() {
    hacking = true;
    hackTimer = 0.5f;  // Hack animation duration
    
    // Set cooldown
    abilityCooldown = Config::HACK_COOLDOWN;
    abilityCooldownRemaining = abilityCooldown;
    
    // Note: Actual hack effect (disabling systems, opening doors) 
    // will be handled in Game.cpp by checking isHacking()
}

// Sera - Berserk Mode
void Player::useBerserk() {
    if (berserkActive) {
        return;  // Already in berserk mode
    }
    
    berserkActive = true;
    berserkTimer = Config::BERSERK_DURATION;
    berserkHealAccumulator = 0.0f;
    
    // Set cooldown
    abilityCooldown = Config::BERSERK_COOLDOWN;
    abilityCooldownRemaining = abilityCooldown;
}

void Player::loadIdleAnimation() {
    auto& spriteManager = SpriteManager::getInstance();
    
    // Load idle animation per character
    if (characterType == CharacterType::Lyra) {
        idleTexturesSouth.clear();
        idleTexturesNorth.clear();
        idleTexturesEast.clear();
        idleTexturesWest.clear();
        
        // Helper lambda to load frames for a direction
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            for (int i = 0; i < 4; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "lyra_idle_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/lyra_pixellab/animations/breathing-idle/" + direction + "/frame_" + frameNum + ".png";
                
                if (spriteManager.loadTexture(id, filepath)) {
                    sf::Texture* tex = spriteManager.getTexture(id);
                    if (tex) {
                        textures.push_back(tex);
                    }
                }
            }
        };
        
        // Load all 4 directions
        loadDirection("south", idleTexturesSouth);
        loadDirection("north", idleTexturesNorth);
        loadDirection("east", idleTexturesEast);
        loadDirection("west", idleTexturesWest);
        
        // If we loaded at least one frame, use sprites
        if (!idleTexturesSouth.empty()) {
            useSprites = true;
            currentAnimationTextures = idleTexturesSouth; // Default to south (face camera)
            sprite.setTexture(*currentAnimationTextures[0]);
            
            // Scale sprite to make it visible (2x = 96x128, 3x = 144x192)
            float spriteScale = 2.0f;
            sprite.setScale(spriteScale, spriteScale);
            
            // Set origin to bottom-center so feet align with hitbox bottom
            // Sprite canvas is 64x64px, but character is only ~38px tall, centered vertically
            sprite.setOrigin(32.0f, 51.0f); // Center X, feet Y position
        }
    }
    else if (characterType == CharacterType::Noah) {
        idleTexturesSouth.clear();
        idleTexturesNorth.clear();
        idleTexturesEast.clear();
        idleTexturesWest.clear();

        auto& sm = spriteManager;
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // Try up to 8 frames; stop when a frame is missing
            for (int i = 0; i < 8; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "noah_idle_" + direction + "_" + std::to_string(i);
                // Expected path for Noah v2 breathing-idle
                std::string filepath = "assets/sprites/noah_pixellab/animations/breathing-idle/" + direction + "/frame_" + frameNum + ".png";
                if (sm.loadTexture(id, filepath)) {
                    if (auto* tex = sm.getTexture(id)) {
                        textures.push_back(tex);
                    }
                } else {
                    // stop at first missing frame for this direction
                    break;
                }
            }
        };

        loadDirection("south", idleTexturesSouth);
        loadDirection("north", idleTexturesNorth);
        loadDirection("east", idleTexturesEast);
        loadDirection("west", idleTexturesWest);

        if (!idleTexturesSouth.empty()) {
            useSprites = true;
            currentAnimationTextures = idleTexturesSouth;
            sprite.setTexture(*currentAnimationTextures[0]);
            // Slightly smaller than Lyra (~-6%)
            float spriteScale = 2.0f * 0.94f;
            sprite.setScale(spriteScale, spriteScale);
            sprite.setOrigin(24.0f, 42.0f);
        }
    }
}

void Player::loadRunAnimation() {
    auto& spriteManager = SpriteManager::getInstance();
    
    // Load run animation per character
    if (characterType == CharacterType::Lyra) {
        runTexturesSouth.clear();
        runTexturesNorth.clear();
        runTexturesEast.clear();
        runTexturesWest.clear();
        
        // Helper lambda to load frames for a direction
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            for (int i = 0; i < 4; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "lyra_run_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/lyra_pixellab/animations/running-4-frames/" + direction + "/frame_" + frameNum + ".png";
                
                if (spriteManager.loadTexture(id, filepath)) {
                    sf::Texture* tex = spriteManager.getTexture(id);
                    if (tex) {
                        textures.push_back(tex);
                    }
                }
            }
        };
        
        // Load all 4 directions
        loadDirection("south", runTexturesSouth);
        loadDirection("north", runTexturesNorth);
        loadDirection("east", runTexturesEast);
        loadDirection("west", runTexturesWest);
    }
    else if (characterType == CharacterType::Noah) {
        runTexturesSouth.clear();
        runTexturesNorth.clear();
        runTexturesEast.clear();
        runTexturesWest.clear();
        
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // running-6-frames has 6 frames
            for (int i = 0; i < 6; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "noah_run_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/noah_pixellab/animations/running-6-frames/" + direction + "/frame_" + frameNum + ".png";
                if (spriteManager.loadTexture(id, filepath)) {
                    if (auto* tex = spriteManager.getTexture(id)) {
                        textures.push_back(tex);
                    }
                } else {
                    break;
                }
            }
        };
        loadDirection("south", runTexturesSouth);
        loadDirection("north", runTexturesNorth);
        loadDirection("east", runTexturesEast);
        loadDirection("west", runTexturesWest);
    }
}

void Player::loadJumpAnimation() {
    auto& spriteManager = SpriteManager::getInstance();
    
    // Load jump animation per character
    if (characterType == CharacterType::Lyra) {
        jumpTexturesSouth.clear();
        jumpTexturesNorth.clear();
        jumpTexturesEast.clear();
        jumpTexturesWest.clear();
        
        // Helper lambda to load frames for a direction
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // jumping-1 has 9 frames (frame_000 to frame_008)
            for (int i = 0; i < 9; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "lyra_jump_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/lyra_pixellab/animations/jumping-1/" + direction + "/frame_" + frameNum + ".png";
                
                if (spriteManager.loadTexture(id, filepath)) {
                    sf::Texture* tex = spriteManager.getTexture(id);
                    if (tex) {
                        textures.push_back(tex);
                    }
                }
            }
        };
        
        // Load all 4 directions
        loadDirection("south", jumpTexturesSouth);
        loadDirection("north", jumpTexturesNorth);
        loadDirection("east", jumpTexturesEast);
        loadDirection("west", jumpTexturesWest);
    }
    else if (characterType == CharacterType::Noah) {
        jumpTexturesSouth.clear();
        jumpTexturesNorth.clear();
        jumpTexturesEast.clear();
        jumpTexturesWest.clear();
        
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // jumping-1 has up to 9 frames
            for (int i = 0; i < 9; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "noah_jump_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/noah_pixellab/animations/jumping-1/" + direction + "/frame_" + frameNum + ".png";
                if (spriteManager.loadTexture(id, filepath)) {
                    if (auto* tex = spriteManager.getTexture(id)) {
                        textures.push_back(tex);
                    }
                } else {
                    break;
                }
            }
        };
        loadDirection("south", jumpTexturesSouth);
        loadDirection("north", jumpTexturesNorth);
        loadDirection("east", jumpTexturesEast);
        loadDirection("west", jumpTexturesWest);
    }
}

void Player::loadDoubleJumpAnimation() {
    auto& spriteManager = SpriteManager::getInstance();
    
    // Load double jump animation for Lyra only for now
    if (characterType == CharacterType::Lyra) {
        doubleJumpTexturesSouth.clear();
        doubleJumpTexturesNorth.clear();
        doubleJumpTexturesEast.clear();
        doubleJumpTexturesWest.clear();
        
        // Helper lambda to load frames for a direction
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // two-footed-jump has 7 frames (frame_000 to frame_006)
            for (int i = 0; i < 7; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "lyra_doublejump_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/lyra_pixellab/animations/two-footed-jump/" + direction + "/frame_" + frameNum + ".png";
                
                if (spriteManager.loadTexture(id, filepath)) {
                    sf::Texture* tex = spriteManager.getTexture(id);
                    if (tex) {
                        textures.push_back(tex);
                    }
                }
            }
        };
        
        // Load all 4 directions
        loadDirection("south", doubleJumpTexturesSouth);
        loadDirection("north", doubleJumpTexturesNorth);
        loadDirection("east", doubleJumpTexturesEast);
        loadDirection("west", doubleJumpTexturesWest);
    }
}

void Player::loadHurtAnimation() {
    auto& spriteManager = SpriteManager::getInstance();
    
    // Load hurt animation per character
    if (characterType == CharacterType::Lyra) {
        hurtTexturesSouth.clear();
        hurtTexturesNorth.clear();
        hurtTexturesEast.clear();
        hurtTexturesWest.clear();
        
        // Helper lambda to load frames for a direction
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // taking-punch has 6 frames (frame_000 to frame_005)
            for (int i = 0; i < 6; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "lyra_hurt_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/lyra_pixellab/animations/taking-punch/" + direction + "/frame_" + frameNum + ".png";
                
                if (spriteManager.loadTexture(id, filepath)) {
                    sf::Texture* tex = spriteManager.getTexture(id);
                    if (tex) {
                        textures.push_back(tex);
                    }
                } else {
                    // If frame doesn't exist, stop loading (likely end of animation)
                    break;
                }
            }
        };
        
        // Load all 4 directions
        loadDirection("south", hurtTexturesSouth);
        loadDirection("north", hurtTexturesNorth);
        loadDirection("east", hurtTexturesEast);
        loadDirection("west", hurtTexturesWest);
    }
    else if (characterType == CharacterType::Noah) {
        hurtTexturesSouth.clear();
        hurtTexturesNorth.clear();
        hurtTexturesEast.clear();
        hurtTexturesWest.clear();
        
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // taking-punch up to 6 frames
            for (int i = 0; i < 6; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "noah_hurt_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/noah_pixellab/animations/taking-punch/" + direction + "/frame_" + frameNum + ".png";
                if (spriteManager.loadTexture(id, filepath)) {
                    if (auto* tex = spriteManager.getTexture(id)) {
                        textures.push_back(tex);
                    }
                } else {
                    break;
                }
            }
        };
        loadDirection("south", hurtTexturesSouth);
        loadDirection("north", hurtTexturesNorth);
        loadDirection("east", hurtTexturesEast);
        loadDirection("west", hurtTexturesWest);
    }
}

void Player::loadDeathAnimation() {
    auto& spriteManager = SpriteManager::getInstance();
    
    // Load death animation per character
    if (characterType == CharacterType::Lyra) {
        deathTexturesSouth.clear();
        deathTexturesNorth.clear();
        deathTexturesEast.clear();
        deathTexturesWest.clear();
        
        // Helper lambda to load frames for a direction
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // falling-back-death has 7 frames (frame_000 to frame_006)
            for (int i = 0; i < 7; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "lyra_death_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/lyra_pixellab/animations/falling-back-death/" + direction + "/frame_" + frameNum + ".png";
                
                if (spriteManager.loadTexture(id, filepath)) {
                    sf::Texture* tex = spriteManager.getTexture(id);
                    if (tex) {
                        textures.push_back(tex);
                    }
                }
            }
        };
        
        // Load all 4 directions
        loadDirection("south", deathTexturesSouth);
        loadDirection("north", deathTexturesNorth);
        loadDirection("east", deathTexturesEast);
        loadDirection("west", deathTexturesWest);
    }
    else if (characterType == CharacterType::Noah) {
        deathTexturesSouth.clear();
        deathTexturesNorth.clear();
        deathTexturesEast.clear();
        deathTexturesWest.clear();
        
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // falling-back-death up to 7 frames
            for (int i = 0; i < 7; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "noah_death_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/noah_pixellab/animations/falling-back-death/" + direction + "/frame_" + frameNum + ".png";
                if (spriteManager.loadTexture(id, filepath)) {
                    if (auto* tex = spriteManager.getTexture(id)) {
                        textures.push_back(tex);
                    }
                } else {
                    break;
                }
            }
        };
        loadDirection("south", deathTexturesSouth);
        loadDirection("north", deathTexturesNorth);
        loadDirection("east", deathTexturesEast);
        loadDirection("west", deathTexturesWest);
    }
}

void Player::loadAbilityAnimation() {
    auto& spriteManager = SpriteManager::getInstance();
    
    // Load ability animation (Kinetic Wave) for Lyra only for now
    if (characterType == CharacterType::Lyra) {
        abilityTexturesSouth.clear();
        abilityTexturesNorth.clear();
        abilityTexturesEast.clear();
        abilityTexturesWest.clear();
        
        // Helper lambda to load frames for a direction
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            // fireball has 6 frames (frame_000 to frame_005)
            for (int i = 0; i < 6; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "lyra_ability_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/lyra_pixellab/animations/fireball/" + direction + "/frame_" + frameNum + ".png";
                
                if (spriteManager.loadTexture(id, filepath)) {
                    sf::Texture* tex = spriteManager.getTexture(id);
                    if (tex) {
                        textures.push_back(tex);
                    }
                }
            }
        };
        
        // Load all 4 directions
        loadDirection("south", abilityTexturesSouth);
        loadDirection("north", abilityTexturesNorth);
        loadDirection("east", abilityTexturesEast);
        loadDirection("west", abilityTexturesWest);
    }
}

void Player::loadKickAnimation() {
    auto& spriteManager = SpriteManager::getInstance();
    
    if (characterType == CharacterType::Lyra) {
        kickTexturesSouth.clear();
        kickTexturesNorth.clear();
        kickTexturesEast.clear();
        kickTexturesWest.clear();
        
        auto loadDirection = [&](const std::string& direction, std::vector<sf::Texture*>& textures) {
            for (int i = 0; i < 7; i++) {
                std::string frameNum = (i < 10 ? "00" : (i < 100 ? "0" : "")) + std::to_string(i);
                std::string id = "lyra_kick_" + direction + "_" + std::to_string(i);
                std::string filepath = "assets/sprites/lyra_pixellab/animations/high-kick/" + direction + "/frame_" + frameNum + ".png";
                
                if (spriteManager.loadTexture(id, filepath)) {
                    sf::Texture* tex = spriteManager.getTexture(id);
                    if (tex) {
                        textures.push_back(tex);
                    }
                } else {
                    break;
                }
            }
        };
        
        loadDirection("south", kickTexturesSouth);
        loadDirection("north", kickTexturesNorth);
        loadDirection("east", kickTexturesEast);
        loadDirection("west", kickTexturesWest);
    }
}

void Player::updateAnimation(float dt) {
    if (currentAnimationTextures.empty()) {
        return;
    }
    
    // Determine current animation state
    AnimationState newState = currentAnimationState;
    
    // Priority: Death > Attack > Ability > Hurt > Jump > Run/Idle
    if (dead) {
        newState = AnimationState::Death;
    } else if (attackAnimationTimer > 0.0f) {
        // Attack animation takes priority when active
        newState = AnimationState::Attack;
    } else if (abilityAnimationTimer > 0.0f) {
        // Ability animation takes priority when active
        newState = AnimationState::Ability;
    } else if (hurtAnimationTimer > 0.0f) {
        // Hurt animation takes priority when active
        newState = AnimationState::Hurt;
    } else if (!isGrounded && velocity.y != 0.0f) {
        // In air - check if double jump or regular jump
        int maxJumps = getMaxJumps();
        if (jumpsRemaining < maxJumps - 1) {
            newState = AnimationState::DoubleJump; // Used air jump = double jump
        } else {
            newState = AnimationState::Jump; // First jump
        }
    } else if (isGrounded) {
        // On ground - check if running
        float speedThreshold = 50.0f; // pixels/s - below this = idle, above = run
        isRunning = std::abs(velocity.x) > speedThreshold;
        
        if (isRunning) {
            newState = AnimationState::Run;
        } else {
            newState = AnimationState::Idle;
        }
    }
    // If neither condition is met, keep current state (transition state)
    
    // Update facing direction based on movement and input
    int newFacingDirection = facingDirection; // Keep last direction by default
    
    // Get input bindings for keys
    const InputBindings& bindings = InputConfig::getInstance().getBindings();
    
    // Check horizontal input (gauche/droite) - priority when moving
    if (velocity.x > 0.1f) {
        newFacingDirection = 1;  // East (right)
    } else if (velocity.x < -0.1f) {
        newFacingDirection = -1; // West (left)
    }
    // Check vertical input (haut/bas) - only when not moving horizontally and on ground
    else if (isGrounded) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || 
            sf::Keyboard::isKeyPressed(bindings.menuUp)) {
            newFacingDirection = 2;  // North (dos/haut)
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || 
                   sf::Keyboard::isKeyPressed(bindings.menuDown)) {
            newFacingDirection = 0;  // South (face/bas)
        }
    }
    
    // Switch direction or animation state if changed
    if (newFacingDirection != facingDirection || newState != currentAnimationState) {
        facingDirection = newFacingDirection;
        currentAnimationState = newState;
        currentAnimationFrame = 0; // Reset to first frame when changing direction or animation
        
        // Update current textures based on state and direction
        const std::vector<sf::Texture*>* texturesToUse = nullptr;
        
        // Helper lambda to get textures for a direction
        auto getDirectionTextures = [&](const std::vector<sf::Texture*>& south, 
                                        const std::vector<sf::Texture*>& north,
                                        const std::vector<sf::Texture*>& east,
                                        const std::vector<sf::Texture*>& west) -> const std::vector<sf::Texture*>* {
            if (facingDirection == 2 && !north.empty()) return &north;
            else if (facingDirection == 1 && !east.empty()) return &east;
            else if (facingDirection == -1 && !west.empty()) return &west;
            else if (!south.empty()) return &south;
            return nullptr;
        };
        
        // Select textures based on animation state
        switch (currentAnimationState) {
            case AnimationState::Death:
                texturesToUse = getDirectionTextures(deathTexturesSouth, deathTexturesNorth,
                                                     deathTexturesEast, deathTexturesWest);
                break;
            case AnimationState::Attack:
                texturesToUse = getDirectionTextures(kickTexturesSouth, kickTexturesNorth,
                                                     kickTexturesEast, kickTexturesWest);
                break;
            case AnimationState::Ability:
                texturesToUse = getDirectionTextures(abilityTexturesSouth, abilityTexturesNorth,
                                                     abilityTexturesEast, abilityTexturesWest);
                break;
            case AnimationState::Hurt:
                texturesToUse = getDirectionTextures(hurtTexturesSouth, hurtTexturesNorth,
                                                     hurtTexturesEast, hurtTexturesWest);
                break;
            case AnimationState::Jump:
                texturesToUse = getDirectionTextures(jumpTexturesSouth, jumpTexturesNorth, 
                                                     jumpTexturesEast, jumpTexturesWest);
                break;
            case AnimationState::DoubleJump:
                texturesToUse = getDirectionTextures(doubleJumpTexturesSouth, doubleJumpTexturesNorth,
                                                     doubleJumpTexturesEast, doubleJumpTexturesWest);
                break;
            case AnimationState::Run:
                texturesToUse = getDirectionTextures(runTexturesSouth, runTexturesNorth,
                                                     runTexturesEast, runTexturesWest);
                break;
            case AnimationState::Idle:
            default:
                texturesToUse = getDirectionTextures(idleTexturesSouth, idleTexturesNorth,
                                                     idleTexturesEast, idleTexturesWest);
                break;
        }
        
        if (texturesToUse && !texturesToUse->empty()) {
            currentAnimationTextures = *texturesToUse;
            sprite.setTexture(*currentAnimationTextures[0]);
        }
    }
    
    // Update animation based on state
    // Death animation: play once, stay on last frame
    if (currentAnimationState == AnimationState::Death) {
        if (!currentAnimationTextures.empty()) {
            animationTimer += dt;
            
            // Play death animation once, then stay on last frame
            if (animationTimer >= deathFrameDuration) {
                animationTimer -= deathFrameDuration;
                if (currentAnimationFrame < static_cast<int>(currentAnimationTextures.size()) - 1) {
                    currentAnimationFrame++;
                    sprite.setTexture(*currentAnimationTextures[currentAnimationFrame]);
                }
                // Otherwise, stay on last frame
            }
        }
    }
    // Attack animation: play once during attack timer
    else if (currentAnimationState == AnimationState::Attack) {
        if (!currentAnimationTextures.empty()) {
            animationTimer += dt;
            
            // Play attack animation once, loop if timer still active
            if (animationTimer >= kickFrameDuration) {
                animationTimer -= kickFrameDuration;
                currentAnimationFrame = (currentAnimationFrame + 1) % currentAnimationTextures.size();
                sprite.setTexture(*currentAnimationTextures[currentAnimationFrame]);
            }
        }
    }
    // Ability animation: play once during ability timer
    else if (currentAnimationState == AnimationState::Ability) {
        if (!currentAnimationTextures.empty()) {
            animationTimer += dt;
            
            // Play ability animation once, loop if timer still active
            if (animationTimer >= abilityFrameDuration) {
                animationTimer -= abilityFrameDuration;
                currentAnimationFrame = (currentAnimationFrame + 1) % currentAnimationTextures.size();
                sprite.setTexture(*currentAnimationTextures[currentAnimationFrame]);
            }
        }
    }
    // Hurt animation: play once during hurt timer
    else if (currentAnimationState == AnimationState::Hurt) {
        if (!currentAnimationTextures.empty()) {
            animationTimer += dt;
            
            // Play hurt animation, loop if timer still active
            if (animationTimer >= hurtFrameDuration) {
                animationTimer -= hurtFrameDuration;
                currentAnimationFrame = (currentAnimationFrame + 1) % currentAnimationTextures.size();
                sprite.setTexture(*currentAnimationTextures[currentAnimationFrame]);
            }
        }
    }
    // Jump animations: sync with velocity (height)
    else if ((currentAnimationState == AnimationState::Jump || currentAnimationState == AnimationState::DoubleJump) && !isGrounded) {
        // For jump animations, sync frame with velocity.y (height of jump)
        // Negative velocity = going up, positive = going down
        if (!currentAnimationTextures.empty()) {
            int totalFrames = static_cast<int>(currentAnimationTextures.size());
            
            // Map velocity to frame: 
            // High negative (ascending) -> early frames (0-2)
            // Near 0 (apex) -> middle frames (3-5)
            // High positive (descending) -> late frames (6-8)
            
            float normalizedVelocity = (velocity.y + 500.0f) / 1000.0f; // Normalize between -500 (max up) and 500 (max down)
            normalizedVelocity = std::max(0.0f, std::min(1.0f, normalizedVelocity)); // Clamp 0-1
            
            // Invert: high negative (going up) should be early frames
            float frameProgress = 1.0f - normalizedVelocity;
            
            int newFrame = static_cast<int>(frameProgress * (totalFrames - 1));
            newFrame = std::max(0, std::min(totalFrames - 1, newFrame));
            
            if (newFrame != currentAnimationFrame) {
                currentAnimationFrame = newFrame;
                sprite.setTexture(*currentAnimationTextures[currentAnimationFrame]);
            }
        }
    } else {
        // For idle/run, use normal frame-by-frame animation
        float frameDuration;
        switch (currentAnimationState) {
            case AnimationState::Run:
                frameDuration = runFrameDuration;
                break;
            case AnimationState::Idle:
            default:
                frameDuration = idleFrameDuration;
                break;
        }
        
        animationTimer += dt;
        
        // Check if we should advance to next frame
        if (animationTimer >= frameDuration && !currentAnimationTextures.empty()) {
            animationTimer -= frameDuration;
            currentAnimationFrame = (currentAnimationFrame + 1) % currentAnimationTextures.size();
            
            // Update sprite texture
            sprite.setTexture(*currentAnimationTextures[currentAnimationFrame]);
        }
    }
}
