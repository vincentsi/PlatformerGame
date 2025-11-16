// Game loop, level management, camera, transitions.
// Central place where input, physics, combat and UI are orchestrated.
#include "core/Game.h"
#include "core/Config.h"
#include "core/InputConfig.h"
#include "core/Logger.h"
#include "core/SaveSystem.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "entities/PatrolEnemy.h"
#include "entities/Spike.h"
#include "entities/FlyingEnemy.h"
#include "entities/KineticWaveProjectile.h"
#include "world/Platform.h"
#include "world/Camera.h"
#include "world/GoalZone.h"
#include "world/Checkpoint.h"
#include "world/InteractiveObject.h"
#include "world/LevelLoader.h"
#include "ui/GameUI.h"
#include "ui/TitleScreen.h"
#include "ui/PauseMenu.h"
#include "ui/SettingsMenu.h"
#include "ui/KeyBindingMenu.h"
#include "effects/ParticleSystem.h"
#include "effects/CameraShake.h"
#include "effects/ScreenTransition.h"
#include "audio/AudioManager.h"
#include "graphics/SpriteManager.h"
#include "physics/CollisionSystem.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <cstdint>

Game::Game()
    : window(sf::VideoMode(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT), Config::WINDOW_TITLE)
    , gameState(GameState::TitleScreen)
    , previousState(GameState::TitleScreen)
    , activePlayerIndex(0)
    , isRunning(true)
    , playerWasDead(false)
    , levelCompleted(false)
    , victoryEffectsTriggered(false)
    , isTransitioning(false)
    , currentLevelNumber(1)
    , secretRoomUnlocked(false)
    , fpsUpdateTime(0.0f)
    , frameCount(0)
    , activeCheckpointId("")
    , bgWallPlain32(nullptr)
    , bgWallCables32(nullptr)
    , bgFarTexture(nullptr)
    , bgWallPlainVarA32(nullptr)
    , bgWallPlainVarB32(nullptr)
    , bgWallCablesAlt32(nullptr)
    , doorKeyHeld(false)
    , lastAbilityTimer(0.0f)
{
    window.setFramerateLimit(Config::FRAMERATE_LIMIT);

    // Initialize logger
    Logger::init("game.log");

    // Create polish systems
    particleSystem = std::make_unique<ParticleSystem>();
    cameraShake = std::make_unique<CameraShake>();
    audioManager = std::make_unique<AudioManager>();
    screenTransition = std::make_unique<ScreenTransition>();

    // Load audio files (optional - game works without them)
    audioManager->loadSound("jump", "assets/sounds/jump.wav");
    audioManager->loadSound("land", "assets/sounds/land.wav");
    audioManager->loadSound("death", "assets/sounds/death.wav");
    audioManager->loadSound("victory", "assets/sounds/victory.wav");
    audioManager->loadSound("checkpoint", "assets/sounds/checkpoint.wav");
    audioManager->loadMusic("gameplay", "assets/music/gameplay.ogg");

    // Create menus
    titleScreen = std::make_unique<TitleScreen>();
    pauseMenu = std::make_unique<PauseMenu>();
    settingsMenu = std::make_unique<SettingsMenu>(audioManager.get());
    keyBindingMenu = std::make_unique<KeyBindingMenu>();

    // Setup menu callbacks
    titleScreen->setCallbacks(
        [this]() { startNewGame(); },
        [this]() { continueGame(); },
        [this]() { setState(GameState::Settings); },
        [this]() { isRunning = false; }
    );

    pauseMenu->setCallbacks(
        [this]() { setState(GameState::Playing); },
        [this]() { setState(GameState::Settings); },
        [this]() { returnToTitleScreen(); },
        [this]() { isRunning = false; }
    );

    settingsMenu->setCallbacks(
        [this]() {
            // Don't update previousState when going to Controls
            // so we can return properly
            gameState = GameState::Controls;
        },
        [this]() {
            // Return to the state before Settings (TitleScreen or Paused)
            setState(previousState);
        }
    );

    keyBindingMenu->setCallbacks(
        [this]() {
            // Always return to Settings from Controls
            gameState = GameState::Settings;
        }
    );

    // Load input configuration
    InputConfig::getInstance().loadFromFile();
    
    // Initialize platform tileset
    Platform::initTileset();

    // Check for existing save
    titleScreen->setCanContinue(SaveSystem::saveExists());

    // Setup FPS counter (optional, will work without font)
    if (Config::SHOW_FPS) {
        // Note: Font loading will fail without a font file, but game will still work
        if (!debugFont.loadFromFile("assets/fonts/arial.ttf")) {
            std::cout << "Warning: Could not load font for FPS display\n";
        } else {
            fpsText.setFont(debugFont);
            fpsText.setCharacterSize(20);
            fpsText.setFillColor(sf::Color::White);
            fpsText.setPosition(10.0f, 10.0f);
        }
    }
}

Game::~Game() {
    // Shutdown logger
    Logger::shutdown();
}

void Game::run() {
    while (window.isOpen() && isRunning) {
        float dt = clock.restart().asSeconds();

        // Cap delta time to avoid spiral of death
        if (dt > Config::MAX_DELTA_TIME) dt = Config::MAX_DELTA_TIME;

        processEvents();
        update(dt);
        render();

        // FPS counter
        if (Config::SHOW_FPS) {
            frameCount++;
            fpsUpdateTime += dt;
            if (fpsUpdateTime >= 1.0f) {
                fpsText.setString("FPS: " + std::to_string(frameCount));
                frameCount = 0;
                fpsUpdateTime = 0.0f;
            }
        }
    }
}

void Game::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }

        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                // Only handle ESC for Playing state (open pause menu)
                // All other states handle ESC through their menu systems
                if (gameState == GameState::Playing) {
                    setState(GameState::Paused);
                    continue; // Don't pass this event to menus
                }
                // Note: All menu states (TitleScreen, Paused, Settings, Controls)
                // handle ESC through their respective menu handleInput() methods
            }
            // Back to previous level with Backspace while playing
            if (event.key.code == sf::Keyboard::BackSpace && gameState == GameState::Playing) {
                goBackOneLevel();
            }

            // Character switch with TAB key (only during gameplay)
            if (event.key.code == sf::Keyboard::Tab && gameState == GameState::Playing) {
                switchCharacter();
            }
        }

        // Handle mouse clicks
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));

            if (gameState == GameState::TitleScreen && titleScreen) {
                titleScreen->handleMouseClick(mousePos);
            } else if (gameState == GameState::Paused && pauseMenu) {
                pauseMenu->handleMouseClick(mousePos);
            } else if (gameState == GameState::Settings && settingsMenu) {
                settingsMenu->handleMouseClick(mousePos);
            } else if (gameState == GameState::Controls && keyBindingMenu) {
                keyBindingMenu->handleMouseClick(mousePos);
            }
        }

        // Pass events to menus
        if (gameState == GameState::TitleScreen && titleScreen) {
            titleScreen->handleInput(event);
        } else if (gameState == GameState::Paused && pauseMenu) {
            pauseMenu->handleInput(event);
        } else if (gameState == GameState::Settings && settingsMenu) {
            settingsMenu->handleInput(event);
        } else if (gameState == GameState::Controls && keyBindingMenu) {
            keyBindingMenu->handleInput(event);
        }
    }

    // Handle mouse movement (for hover effects)
    if (window.hasFocus()) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos(static_cast<float>(mousePixelPos.x), static_cast<float>(mousePixelPos.y));

        if (gameState == GameState::TitleScreen && titleScreen) {
            titleScreen->handleMouseMove(mousePos);
        } else if (gameState == GameState::Paused && pauseMenu) {
            pauseMenu->handleMouseMove(mousePos);
        } else if (gameState == GameState::Settings && settingsMenu) {
            settingsMenu->handleMouseMove(mousePos);
        } else if (gameState == GameState::Controls && keyBindingMenu) {
            keyBindingMenu->handleMouseMove(mousePos);
        }
    }

    // Handle gameplay input only when playing
    if (gameState == GameState::Playing) {
        handleInput();
    }
}

void Game::handleInput() {
    const InputBindings& bindings = InputConfig::getInstance().getBindings();
    Player* player = getActivePlayer();
    if (!player) return;

    // Disable inputs while dead (no movement or actions during death animation)
    if (player->isDead()) {
        // lock horizontal velocity
        sf::Vector2f v = player->getVelocity();
        player->setVelocity(0.0f, v.y);
        return;
    }

    // Jump (check every frame for better responsiveness)
    if (sf::Keyboard::isKeyPressed(bindings.jump)) {
        player->jump();
    }

    // Special ability (one-shot activation on key press)
    static bool abilityKeyPressed = false;
    if (sf::Keyboard::isKeyPressed(bindings.ability)) {
        if (!abilityKeyPressed && player->canUseAbility()) {
            player->useAbility();
            abilityKeyPressed = true;
        }
    } else {
        abilityKeyPressed = false;
    }

    // Horizontal movement
    if (sf::Keyboard::isKeyPressed(bindings.moveLeft) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        player->moveLeft();
    }
    else if (sf::Keyboard::isKeyPressed(bindings.moveRight) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        player->moveRight();
    }
    else {
        player->stopMoving();
    }
}

void Game::update(float dt) {
    if (gameState == GameState::TitleScreen && titleScreen) {
        titleScreen->update(dt);
        return;
    } else if (gameState == GameState::Paused && pauseMenu) {
        pauseMenu->update(dt);
        return;
    } else if (gameState == GameState::Settings && settingsMenu) {
        settingsMenu->update(dt);
        return;
    } else if (gameState == GameState::Controls && keyBindingMenu) {
        keyBindingMenu->update(dt);
        return;
    }

    if (gameState != GameState::Playing && gameState != GameState::Transitioning) {
        return;
    }

    if (isTransitioning) {
        screenTransition->update(dt);

        if (screenTransition->isFadedOut() && !nextLevelPath.empty()) {
            std::string previousLevelPath = currentLevelPath;
            std::string previousCheckpoint = activeCheckpointId;
            
            if (!previousLevelPath.empty()) {
                levelCheckpoints[previousLevelPath] = previousCheckpoint;
            }
            
            loadLevel(nextLevelPath);
            nextLevelPath = "";
            
            if (pendingSpawnEdge == 1) {
                if (levelHistoryPos > 0) {
                    levelHistoryPos--;
                }
            } else {
                if (!previousLevelPath.empty() && (levelHistory.empty() || levelHistory.back() != previousLevelPath)) {
                    levelHistory.push_back(previousLevelPath);
                }
                levelHistory.push_back(currentLevelPath);
                levelHistoryPos = static_cast<int>(levelHistory.size()) - 1;
            }
            
            auto it = levelCheckpoints.find(currentLevelPath);
            if (it != levelCheckpoints.end() && !it->second.empty()) {
                activeCheckpointId = it->second;
                for (auto& checkpoint : checkpoints) {
                    if (checkpoint->getId() == activeCheckpointId) {
                        checkpoint->activate();
                        break;
                    }
                }
            }
            
            screenTransition->startFadeIn(0.9f);
            levelCompleted = false;
            victoryEffectsTriggered = false;
        }

        if (screenTransition->isComplete()) {
            isTransitioning = false;
            postTransitionHideFrames = 2;
        }

        return;
    }

    Player* player = getActivePlayer();
    if (!player) {
        return;
    }

    if (postTransitionHideFrames > 0) {
        postTransitionHideFrames--;
    }

    player->update(dt);

    sf::FloatRect playerBounds = player->getBounds();
    sf::Vector2f playerVel = player->getVelocity();
    bool grounded = false;

    for (auto& platform : platforms) {
        sf::FloatRect platformBounds = platform->getBounds();
        if (CollisionSystem::resolveCollision(
            playerBounds,
            playerVel,
            platformBounds,
            grounded
        )) {
            player->setPosition(playerBounds.left, playerBounds.top);
            player->setVelocity(playerVel);
            
        }
    }

    player->setGrounded(grounded);

    if (player->hasJustJumped()) {
        sf::Vector2f playerPos = player->getPosition();
        particleSystem->emitJump(sf::Vector2f(playerPos.x + 20.0f, playerPos.y + 40.0f));
        audioManager->playSound("jump", 80.0f);
    }

    if (player->hasJustLanded()) {
        sf::Vector2f playerPos = player->getPosition();
        particleSystem->emitLanding(sf::Vector2f(playerPos.x + 20.0f, playerPos.y + 40.0f));
        audioManager->playSound("land", 60.0f);
        cameraShake->shakeLight();
    }

    player->clearEventFlags();

    gameUI->setHealth(player->getHealth(), player->getMaxHealth());

    if (player->isDead() && !playerWasDead) {
        gameUI->incrementDeaths();
        playerWasDead = true;

        // Death effects
        sf::Vector2f playerPos = player->getPosition();
        particleSystem->emitDeath(sf::Vector2f(playerPos.x + 20.0f, playerPos.y + 20.0f));
        audioManager->playSound("death", 100.0f);
        cameraShake->shakeMedium();
    }
    else if (!player->isDead() && playerWasDead) {
        playerWasDead = false;
    }

    // Update checkpoints
    for (auto& checkpoint : checkpoints) {
        checkpoint->update(dt);

        // Check if player touched checkpoint
        if (!checkpoint->isActivated() && checkpoint->isPlayerInside(player->getBounds())) {
            checkpoint->activate();
            activeCheckpointId = checkpoint->getId();
            if (!currentLevelPath.empty()) {
                levelCheckpoints[currentLevelPath] = activeCheckpointId;
            }
            player->setSpawnPoint(checkpoint->getSpawnPosition().x, checkpoint->getSpawnPosition().y);
            audioManager->playSound("checkpoint", 70.0f);

            // Visual feedback
            sf::Vector2f cpPos = checkpoint->getSpawnPosition();
            particleSystem->emitVictory(sf::Vector2f(cpPos.x + 20.0f, cpPos.y + 30.0f));
        }
    }

    // Update interactive objects
    for (auto& interactive : interactiveObjects) {
        if (interactive) {
            interactive->update(dt);
        }
    }

    // Edge-based level transitions (screen edges act like doors)
    if (!isTransitioning && currentLevel && !currentLevel->cameraZones.empty()) {
        const auto& zone = currentLevel->cameraZones[0];
        float leftEdge = zone.minX;
        float rightEdge = zone.maxX;
        float x = player->getPosition().x;
        float margin = 8.0f;

        // Go to previous level if player exits left and a history exists
        if (x < leftEdge - margin && levelHistoryPos > 0) {
            goBackOneLevel();
            return;
        }

        // Go to next level if player exits right and there is a next level defined
        if (x > rightEdge + margin) {
            // Determine next level from level graph if available
            if (currentLevel && !currentLevel->nextLevels.empty()) {
                std::string nextLevelId = currentLevel->nextLevels[0];
                nextLevelPath = "assets/levels/" + nextLevelId + ".json";
            } else {
                // Fallback: legacy linear progression
                currentLevelNumber++;
                nextLevelPath = "assets/levels/level" + std::to_string(currentLevelNumber) + ".json";
            }

            pendingSpawnEdge = 0; // appear at left edge of next level
            isTransitioning = true;
            // Fade out encore plus long (transition plus douce)
            screenTransition->startFadeOut(1.3f);
            return;
        }
    }

    // Door backtracking: if player presses Up/W inside a Door, go back one level
    {
        const bool keyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
                                sf::Keyboard::isKeyPressed(sf::Keyboard::W);
        if (!doorKeyHeld && keyPressed && !isTransitioning && player && levelHistoryPos > 0) {
            for (auto& interactive : interactiveObjects) {
                if (!interactive) continue;
                if (interactive->getType() != InteractiveType::Door) continue;
                if (player->getBounds().intersects(interactive->getBounds())) {
                    goBackOneLevel();
                    break;
                }
            }
        }
        doorKeyHeld = keyPressed;
    }

    // Handle Noah's Hack ability (interact with terminals, doors, etc.)
    if (player && player->isHacking() && player->getCharacterType() == CharacterType::Noah) {
        for (auto& interactive : interactiveObjects) {
            if (!interactive) continue;
            
            // Check if player is in range
            if (interactive->isPlayerInRange(player->getBounds())) {
                // Activate the interactive object
                if (!interactive->isActivated()) {
                    interactive->activate();
                    
                    // Visual feedback
                    sf::Vector2f objPos = interactive->getPosition();
                    particleSystem->emitVictory(sf::Vector2f(objPos.x + interactive->getSize().x / 2.0f, objPos.y + interactive->getSize().y / 2.0f));
                    audioManager->playSound("checkpoint", 70.0f);
                    cameraShake->shakeLight();
                    
                    // Handle different types of interactive objects
                    if (interactive->getType() == InteractiveType::Door) {
                        // Door: Make platform passable or remove it
                        // For now, just mark as activated (can be used to unlock doors later)
                        std::cout << "Door " << interactive->getId() << " hacked!\n";
                    } else if (interactive->getType() == InteractiveType::Terminal) {
                        // Terminal: Activate systems, unlock paths
                        std::cout << "Terminal " << interactive->getId() << " hacked!\n";
                    } else if (interactive->getType() == InteractiveType::Turret) {
                        // Turret: Disable turret
                        std::cout << "Turret " << interactive->getId() << " disabled!\n";
                    }
                }
            }
        }
    }

    // Handle Lyra's Kinetic Wave ability - create projectile during animation
    if (player && player->getCharacterType() == CharacterType::Lyra) {
        float abilityTimer = player->getAbilityAnimationTimer();
        
        // Create projectile very late in ability animation (when projectile is actually launched)
        // Ability animation is 0.6s total, launch projectile at the very end (almost finished)
        // Only create once when timer crosses threshold (when animation is almost done)
        if (abilityTimer > 0.05f && abilityTimer <= 0.1f && lastAbilityTimer > 0.1f) {
            // Timer is very low (animation almost finished), create projectile
            sf::Vector2f playerPos = player->getPosition();
            sf::Vector2f waveDir = player->getKineticWaveDirection();
            float waveRange = Config::KINETIC_WAVE_RANGE;
            
            // Create moving projectile that travels forward
            // Position at hands level (chest/torso area, aligned with arms)
            // Player hitbox center is around torso, hands are at chest level (not head)
            // Use positive Y offset to lower projectile (closer to ground, at hands level)
            float handsOffsetX = waveDir.x * 25.0f; // Forward in direction
            float handsOffsetY = 20.0f; // Below center (hands level, lower than before)
            sf::Vector2f waveStartPos = playerPos + sf::Vector2f(handsOffsetX, handsOffsetY);
            float projectileSpeed = 800.0f; // pixels per second
            kineticWaveProjectiles.push_back(
                std::make_unique<KineticWaveProjectile>(waveStartPos, waveDir, projectileSpeed, waveRange)
            );
        }
        lastAbilityTimer = abilityTimer;

        // Reset tracking when ability ends
        if (abilityTimer <= 0.0f) {
            lastAbilityTimer = 0.0f;
        }
    }
    
    // Build a simple spatial grid for enemies to reduce projectile/enemy checks
    const float cellSize = 128.0f;
    std::unordered_map<std::int64_t, std::vector<Enemy*>> enemyGrid;
    enemyGrid.reserve(enemies.size() * 2);

    auto makeCellKey = [](int cx, int cy) -> std::int64_t {
        return (static_cast<std::int64_t>(cx) << 32) ^
               (static_cast<std::int64_t>(cy) & 0xffffffffLL);
    };

    for (auto& enemyPtr : enemies) {
        if (!enemyPtr || !enemyPtr->isAlive()) {
            continue;
        }
        const sf::Vector2f& enemyPos = enemyPtr->getPosition();
        const int cx = static_cast<int>(std::floor(enemyPos.x / cellSize));
        const int cy = static_cast<int>(std::floor(enemyPos.y / cellSize));
        enemyGrid[makeCellKey(cx, cy)].push_back(enemyPtr.get());
    }

    // Update Kinetic Wave projectiles
    for (auto& projectile : kineticWaveProjectiles) {
        if (!projectile || !projectile->isAlive()) {
            continue;
        }

        projectile->update(dt);

        // Broad phase: only test enemies in neighboring grid cells
        const sf::Vector2f& projectilePos = projectile->getPosition();
        const int pcx = static_cast<int>(std::floor(projectilePos.x / cellSize));
        const int pcy = static_cast<int>(std::floor(projectilePos.y / cellSize));

        for (int gx = pcx - 1; gx <= pcx + 1; ++gx) {
            for (int gy = pcy - 1; gy <= pcy + 1; ++gy) {
                auto it = enemyGrid.find(makeCellKey(gx, gy));
                if (it == enemyGrid.end()) {
                    continue;
                }

                for (Enemy* enemy : it->second) {
                    if (!enemy || !enemy->isAlive()) {
                        continue;
                    }

                    const sf::Vector2f& enemyPos = enemy->getPosition();
                    sf::Vector2f toEnemy = enemyPos - projectilePos;
                    float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y);

                    // Check collision (projectile radius ~15px, enemy hitbox)
                    if (distance < 40.0f) { // Slightly larger collision radius
                        // Push enemy away with force
                        sf::Vector2f dirToEnemy = (distance > 0.0f)
                            ? sf::Vector2f(toEnemy.x / distance, toEnemy.y / distance)
                            : sf::Vector2f(1.0f, 0.0f);
                        sf::Vector2f pushForce = dirToEnemy * Config::KINETIC_WAVE_FORCE;
                        enemy->setVelocity(pushForce.x, pushForce.y);

                        // Visual effect at enemy position (impact)
                        particleSystem->emitJump(enemyPos);
                        audioManager->playSound("jump", 60.0f);
                        cameraShake->shakeLight();

                        // Mark projectile as dead after hitting enemy
                        // (we'll add a hit flag later if needed to continue through enemies)
                    }
                }
            }
        }
    }
    
    // Remove dead projectiles (swap-and-pop to avoid extra allocations)
    for (std::size_t i = 0; i < kineticWaveProjectiles.size(); ) {
        if (!kineticWaveProjectiles[i] || !kineticWaveProjectiles[i]->isAlive()) {
            kineticWaveProjectiles[i] = std::move(kineticWaveProjectiles.back());
            kineticWaveProjectiles.pop_back();
        } else {
            ++i;
        }
    }

    // Update enemies
    for (auto& enemy : enemies) {
        if (!enemy || !enemy->isAlive()) {
            continue; // Skip null or dead enemies (will be removed later)
        }

        enemy->update(dt);

        // Check collision with player
        if (player && player->getBounds().intersects(enemy->getBounds())) {
            sf::FloatRect currentPlayerBounds = player->getBounds();
            sf::FloatRect enemyBounds = enemy->getBounds();

            // Spikes (Stationary enemies) cannot be stomped - they always deal damage
            if (enemy->getType() == EnemyType::Stationary) {
                // Always take damage from spikes (no stomping)
                if (!player->isInvincible()) {
                    player->takeDamage(1);

                    // Play hurt sound if player is still alive
                    if (!player->isDead()) {
                        audioManager->playSound("jump", 60.0f); // Temporary hurt sound
                        cameraShake->shakeLight();
                    }
                }
            } else {
                // For other enemies: Check if player is stomping on enemy (falling and hitting from above)
                float tolerance = Config::STOMP_TOLERANCE_BASE * player->getStompDamageMultiplier();  // Noah has bigger stomp range
                bool playerFalling = player->getVelocity().y > 0;
                bool hitFromAbove = currentPlayerBounds.top + currentPlayerBounds.height <= enemyBounds.top + tolerance;

                if (playerFalling && hitFromAbove) {
                    // Zone 1 Level 1: Flying enemy allows bounce without killing (for secret)
                    if (currentLevel && currentLevel->levelId == "zone1_level1" && enemy->getType() == EnemyType::Flying) {
                        // Don't kill flying enemy, just bounce (allows multiple uses for secret)
                        // Bounce player up higher for secret access
                        player->setVelocity(player->getVelocity().x, Config::FLYING_ENEMY_BOUNCE_VELOCITY);
                        
                        // Effects
                        sf::Vector2f enemyPos = enemy->getPosition();
                        particleSystem->emitJump(sf::Vector2f(enemyPos.x + 15.0f, enemyPos.y + 15.0f));
                        audioManager->playSound("jump", 80.0f);
                        cameraShake->shakeLight();
                    } else {
                        // Kill enemy (normal behavior)
                        enemy->kill();

                        // Bounce player up
                        player->setVelocity(player->getVelocity().x, Config::ENEMY_BOUNCE_VELOCITY);

                        // Effects
                        sf::Vector2f enemyPos = enemy->getPosition();
                        particleSystem->emitDeath(sf::Vector2f(enemyPos.x + 15.0f, enemyPos.y + 15.0f));
                        audioManager->playSound("death", 80.0f);
                        cameraShake->shakeLight();
                    }
                } else {
                    // Side or bottom collision = player takes damage
                    // Only play effects if player is not invincible (takeDamage will handle invincibility check)
                    if (!player->isInvincible()) {
                        player->takeDamage(1);

                        // Play hurt sound if player is still alive
                        if (!player->isDead()) {
                            audioManager->playSound("jump", 60.0f); // Temporary hurt sound
                            cameraShake->shakeLight();
                        }
                    }
                }
            }
        }
    }

    // Remove dead enemies from vector (cleanup after update)
    enemies.erase(
        std::remove_if(enemies.begin(), enemies.end(),
            [](const std::unique_ptr<Enemy>& enemy) -> bool {
                return !enemy || !enemy->isAlive();
            }),
        enemies.end()
    );

    // GoalZone disabled (portals no longer used; edges handle transitions)
    (void)goalZone;

    // Victory effects (only trigger once)
    if (levelCompleted && !victoryEffectsTriggered) {
        sf::Vector2f playerPos = player->getPosition();
        particleSystem->emitVictory(sf::Vector2f(playerPos.x + 20.0f, playerPos.y + 20.0f));
        audioManager->playSound("victory", 100.0f);
        victoryEffectsTriggered = true;
    }

    // Update polish systems
    particleSystem->update(dt);
    cameraShake->update(dt);

    // Update camera to follow player
    camera->update(player->getPosition(), dt);
    camera->setShakeOffset(cameraShake->getOffset());

    // Update UI
    gameUI->update(dt);
}

void Game::render() {
    window.clear(sf::Color(13, 27, 42));

    if (gameState == GameState::TitleScreen && titleScreen) {
        titleScreen->draw(window);
        window.display();
        return;
    } else if (gameState == GameState::Paused && pauseMenu) {
        Player* player = getActivePlayer();
        if (camera && player) {
            camera->apply(window);
            for (auto& platform : platforms) platform->draw(window);
            for (auto& checkpoint : checkpoints) checkpoint->draw(window);
            for (auto& interactive : interactiveObjects) {
                if (interactive) interactive->draw(window);
            }
            if (goalZone) goalZone->draw(window);
            for (auto& enemy : enemies) {
                if (enemy && enemy->isAlive()) {
                    enemy->draw(window);
                }
            }
            particleSystem->draw(window);
            player->draw(window);
            window.setView(window.getDefaultView());
            if (gameUI) gameUI->draw(window);
        }
        pauseMenu->draw(window);
        window.display();
        return;
    } else if (gameState == GameState::Settings && settingsMenu) {
        settingsMenu->draw(window);
        window.display();
        return;
    } else if (gameState == GameState::Controls && keyBindingMenu) {
        keyBindingMenu->draw(window);
        window.display();
        return;
    }

    Player* player = getActivePlayer();
    bool shouldRenderGameplay = (gameState == GameState::Playing && camera && player) || 
                                (isTransitioning && camera && player);
    
    if (shouldRenderGameplay) {
        camera->apply(window);

        drawParallaxBackground(window);

        for (auto& platform : platforms) {
            platform->draw(window);
        }

        for (auto& checkpoint : checkpoints) {
            checkpoint->draw(window);
        }

        // for (auto& interactive : interactiveObjects) {
        //     if (interactive) {
        //         interactive->draw(window);
        //     }
        // }

        for (auto& enemy : enemies) {
            if (enemy && enemy->isAlive()) {
                enemy->draw(window);
            }
        }

        for (const auto& projectile : kineticWaveProjectiles) {
            if (projectile && projectile->isAlive()) {
                projectile->draw(window);
            }
        }

        particleSystem->draw(window);

        if (player && !isTransitioning && postTransitionHideFrames == 0) {
            player->draw(window);
        }

        window.setView(window.getDefaultView());

        if (gameUI) {
            gameUI->draw(window);
        }

        if (Config::SHOW_FPS && debugFont.getInfo().family != "") {
            window.draw(fpsText);
        }

        screenTransition->draw(window);
    }

    window.display();
}

void Game::drawParallaxBackground(sf::RenderWindow& renderWindow) {
    if (!currentLevel || !camera) return;

    if (currentLevel->zoneNumber == 1 && bgWallPlain32) {
        sf::View view = renderWindow.getView();
        sf::Vector2f center = view.getCenter();
        sf::Vector2f size = view.getSize();

        float left = center.x - size.x / 2.0f;
        float right = center.x + size.x / 2.0f;
        float top = center.y - size.y / 2.0f;
        float bottom = center.y + size.y / 2.0f;

        const float tileSize = 32.0f;
        float startY = std::floor(top / tileSize) * tileSize;
        float endY = std::ceil(bottom / tileSize) * tileSize;
        float startX = std::floor(left / tileSize) * tileSize;
        float endX = std::ceil(right / tileSize) * tileSize;

        // Tuilage sans espace: on rogne automatiquement la texture pour enlever le padding transparent
        static bool trimComputed = false;
        static sf::IntRect trimRect(0,0,0,0);
        if (!trimComputed) {
            if (bgWallPlain32) {
                sf::Image img = bgWallPlain32->copyToImage();
                sf::Vector2u sz = img.getSize();
                unsigned int minX = sz.x, minY = sz.y, maxX = 0, maxY = 0;
                bool any = false;
                for (unsigned int y = 0; y < sz.y; ++y) {
                    for (unsigned int x = 0; x < sz.x; ++x) {
                        if (img.getPixel(x, y).a > 0) {
                            any = true;
                            if (x < minX) minX = x;
                            if (y < minY) minY = y;
                            if (x > maxX) maxX = x;
                            if (y > maxY) maxY = y;
                        }
                    }
                }
                if (any) {
                    trimRect = sf::IntRect(static_cast<int>(minX), static_cast<int>(minY),
                                           static_cast<int>(maxX - minX + 1), static_cast<int>(maxY - minY + 1));
                } else {
                    trimRect = sf::IntRect(0,0,static_cast<int>(tileSize), static_cast<int>(tileSize));
                }
            }
            trimComputed = true;
        }

        sf::Sprite sprite;
        sprite.setTexture(*bgWallPlain32);
        if (trimRect.width > 0 && trimRect.height > 0) {
            sprite.setTextureRect(trimRect);
            float scaleX = tileSize / static_cast<float>(trimRect.width);
            float scaleY = tileSize / static_cast<float>(trimRect.height);
            sprite.setScale(scaleX, scaleY);
        } else {
            sprite.setScale(1.0f, 1.0f);
            sprite.setTextureRect(sf::IntRect(0, 0, static_cast<int>(tileSize), static_cast<int>(tileSize)));
        }

        for (float x = startX; x < endX; x += tileSize) {
            for (float y = startY; y < endY; y += tileSize) {
                sprite.setPosition(std::floor(x), std::floor(y));
                renderWindow.draw(sprite);
            }
        }
    }
}

void Game::loadLevel() {
    // Load first level of Zone 1 (Salle d'Éveil)
    loadLevel("assets/levels/zone1_level1.json");
}

void Game::loadLevel(const std::string& levelPath) {
    // Load level from specified path
    currentLevel = LevelLoader::loadFromFile(levelPath);
    currentLevelPath = levelPath;

        if (currentLevel) {
            // Move data from LevelData to Game
            platforms = std::move(currentLevel->platforms);
            checkpoints = std::move(currentLevel->checkpoints);
            interactiveObjects = std::move(currentLevel->interactiveObjects);
            goalZone = std::move(currentLevel->goalZone);

        // Clear enemies and projectiles, then add level-specific enemies
        enemies.clear();
        kineticWaveProjectiles.clear();
        
        // Zone 1 Level 1: Laboratoire - Premier monstre volant
        if (currentLevel->levelId == "zone1_level1") {
            // Flying enemy horizontal (de gauche à droite) au-dessus de la plateforme
            // Position: au-dessus de la plateforme à y=400, patrouille horizontalement
            enemies.push_back(std::make_unique<FlyingEnemy>(600.0f, 400.0f, 800.0f, true));
        }

        // Zone 1 Level 2: Couloir - Ennemis et pièges denses (style Dead Cells)
        if (currentLevel->levelId == "zone1_level2") {
            // Ennemis patrouilleurs sur les plateformes (densité élevée)
            enemies.push_back(std::make_unique<PatrolEnemy>(300.0f, 230.0f, 80.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(550.0f, 470.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(600.0f, 80.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(800.0f, 420.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(1100.0f, 70.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(1200.0f, 320.0f, 120.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(1500.0f, 190.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(1800.0f, 220.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(1900.0f, 150.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(2300.0f, 120.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(2600.0f, 80.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(3100.0f, 170.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(3300.0f, 30.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(3600.0f, 270.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(3700.0f, 110.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(4100.0f, 370.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(4050.0f, 50.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(4500.0f, 80.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(4700.0f, 180.0f, 100.0f));
            enemies.push_back(std::make_unique<PatrolEnemy>(5000.0f, 330.0f, 100.0f));

            // Flying enemies pour la verticalité
            enemies.push_back(std::make_unique<FlyingEnemy>(700.0f, 300.0f, 200.0f, true));
            enemies.push_back(std::make_unique<FlyingEnemy>(1600.0f, 200.0f, 200.0f, true));
            enemies.push_back(std::make_unique<FlyingEnemy>(2800.0f, 100.0f, 200.0f, true));
            enemies.push_back(std::make_unique<FlyingEnemy>(3900.0f, 200.0f, 200.0f, true));
            enemies.push_back(std::make_unique<FlyingEnemy>(4800.0f, 300.0f, 200.0f, true));

            // Pièges (spikes) stratégiquement placés partout
            enemies.push_back(std::make_unique<Spike>(400.0f, 530.0f));
            enemies.push_back(std::make_unique<Spike>(650.0f, 480.0f));
            enemies.push_back(std::make_unique<Spike>(900.0f, 430.0f));
            enemies.push_back(std::make_unique<Spike>(1100.0f, 380.0f));
            enemies.push_back(std::make_unique<Spike>(1300.0f, 330.0f));
            enemies.push_back(std::make_unique<Spike>(1600.0f, 280.0f));
            enemies.push_back(std::make_unique<Spike>(1950.0f, 230.0f));
            enemies.push_back(std::make_unique<Spike>(2200.0f, 180.0f));
            enemies.push_back(std::make_unique<Spike>(2500.0f, 80.0f));
            enemies.push_back(std::make_unique<Spike>(2750.0f, 80.0f));
            enemies.push_back(std::make_unique<Spike>(3100.0f, 180.0f));
            enemies.push_back(std::make_unique<Spike>(3400.0f, 230.0f));
            enemies.push_back(std::make_unique<Spike>(3500.0f, 230.0f));
            enemies.push_back(std::make_unique<Spike>(3800.0f, 330.0f));
            enemies.push_back(std::make_unique<Spike>(4000.0f, 380.0f));
            enemies.push_back(std::make_unique<Spike>(4300.0f, 430.0f));
            enemies.push_back(std::make_unique<Spike>(4600.0f, 130.0f));
            enemies.push_back(std::make_unique<Spike>(4900.0f, 280.0f));
            enemies.push_back(std::make_unique<Spike>(5100.0f, 380.0f));
        }

        // Reset all players to level start position (or edge spawn if requested)
        if (!players.empty()) {
            sf::Vector2f spawnPos = currentLevel->startPosition;
            if (pendingSpawnEdge != -1 && !currentLevel->cameraZones.empty()) {
                const auto& zone = currentLevel->cameraZones[0];
                if (pendingSpawnEdge == 0) {
                    // left edge spawn
                    spawnPos.x = zone.minX + 80.0f;
                } else if (pendingSpawnEdge == 1) {
                    // right edge spawn
                    spawnPos.x = zone.maxX - 80.0f;
                }
                // y stays from level start to ensure on ground
            }
            for (auto& p : players) {
                if (p) {
                    p->setPosition(spawnPos.x, spawnPos.y);
                    p->setSpawnPoint(spawnPos.x, spawnPos.y);
                    p->setVelocity(sf::Vector2f(0.0f, 0.0f));
                }
            }
            pendingSpawnEdge = -1; // consume
        }

        // Restore checkpoint state for this level
        auto it = levelCheckpoints.find(currentLevelPath);
        if (it != levelCheckpoints.end() && !it->second.empty()) {
            activeCheckpointId = it->second;
            for (auto& checkpoint : checkpoints) {
                if (checkpoint->getId() == activeCheckpointId) {
                    checkpoint->activate();
                    break;
                }
            }
        } else {
            activeCheckpointId.clear();
        }

        // Reset level state
        levelCompleted = false;
        victoryEffectsTriggered = false;
        secretRoomUnlocked = false;

        // Update camera limits (only if camera exists)
        if (camera && !currentLevel->cameraZones.empty()) {
            const auto& zone = currentLevel->cameraZones[0];
            camera->setLimits(zone.minX, zone.maxX, zone.minY, zone.maxY);
        }

        // Reset camera to active player position immediately (use update with 0 dt for instant snap)
        // Only if camera and player exist
        if (camera) {
            Player* activePlayer = getActivePlayer();
            if (activePlayer) {
                camera->update(activePlayer->getPosition(), 0.0f);
            }
        }

        // Reset UI (only if UI exists)
        if (gameUI) {
            gameUI->hideVictoryMessage();
        }

        // Load background wall textures (parallax far + simple 32x32 tiling)
        SpriteManager& sm = SpriteManager::getInstance();
        if (currentLevel->zoneNumber == 1) {
            if (sm.loadTexture("zone1_bg_far_dark_256", "assets/backgrounds/zone1/zone1_bg_far_dark_256.png")) {
                bgFarTexture = sm.getTexture("zone1_bg_far_dark_256");
            } else {
                bgFarTexture = nullptr;
            }
            if (sm.loadTexture("zone1_bg_wall_plain_32", "assets/backgrounds/zone1/zone1_bg_wall_plain_32.png")) {
                bgWallPlain32 = sm.getTexture("zone1_bg_wall_plain_32");
            }
            if (sm.loadTexture("zone1_bg_wall_plain_varA_32", "assets/backgrounds/zone1/zone1_bg_wall_plain_varA_32.png")) {
                bgWallPlainVarA32 = sm.getTexture("zone1_bg_wall_plain_varA_32");
            }
            if (sm.loadTexture("zone1_bg_wall_plain_varB_32", "assets/backgrounds/zone1/zone1_bg_wall_plain_varB_32.png")) {
                bgWallPlainVarB32 = sm.getTexture("zone1_bg_wall_plain_varB_32");
            }
            if (sm.loadTexture("zone1_bg_wall_cables_32", "assets/backgrounds/zone1/zone1_bg_wall_cables_32.png")) {
                bgWallCables32 = sm.getTexture("zone1_bg_wall_cables_32");
            }
            if (sm.loadTexture("zone1_bg_wall_cables_alt_32", "assets/backgrounds/zone1/zone1_bg_wall_cables_alt_32.png")) {
                bgWallCablesAlt32 = sm.getTexture("zone1_bg_wall_cables_alt_32");
            }
        }

        std::cout << "Level loaded: " << currentLevel->name << "\n";
    }
}

void Game::goBackOneLevel() {
    if (levelHistoryPos <= 0 || isTransitioning) {
        return;
    }
    nextLevelPath = levelHistory[levelHistoryPos - 1];
    pendingSpawnEdge = 1;
    isTransitioning = true;
    screenTransition->startFadeOut(1.3f);
}

void Game::startNewGame() {
    std::cout << "Starting new game\n";

    // Reset save data
    saveData = SaveData();
    saveData.currentLevel = 1;
    currentLevelNumber = 1;
    levelHistory.clear();
    levelHistoryPos = -1;
    currentLevelPath.clear();

    // Load level first
    loadLevel();
    if (!currentLevelPath.empty()) {
        levelHistory.push_back(currentLevelPath);
        levelHistoryPos = 0;
    }

    // Create all 3 characters at level start position
    sf::Vector2f startPos = currentLevel ? currentLevel->startPosition : sf::Vector2f(100.0f, 100.0f);

    // Create Lyra (Green) - Starting character
    auto lyra = std::make_unique<Player>(startPos.x, startPos.y, CharacterType::Lyra);
    lyra->setSpawnPoint(startPos.x, startPos.y);
    players.push_back(std::move(lyra));

    // Create Noah (Blue) - Second character at same position
    auto noah = std::make_unique<Player>(startPos.x, startPos.y, CharacterType::Noah);
    noah->setSpawnPoint(startPos.x, startPos.y);
    players.push_back(std::move(noah));

    // Create Sera (Magenta) - Third character at same position
    auto sera = std::make_unique<Player>(startPos.x, startPos.y, CharacterType::Sera);
    sera->setSpawnPoint(startPos.x, startPos.y);
    players.push_back(std::move(sera));

    // Start with Lyra (index 0)
    activePlayerIndex = 0;

    // Create camera
    camera = std::make_unique<Camera>(
        static_cast<float>(Config::WINDOW_WIDTH),
        static_cast<float>(Config::WINDOW_HEIGHT)
    );

    // Set camera limits if level has zones
    if (currentLevel && !currentLevel->cameraZones.empty()) {
        const auto& zone = currentLevel->cameraZones[0];
        camera->setLimits(zone.minX, zone.maxX, zone.minY, zone.maxY);
    }

    // Create UI
    gameUI = std::make_unique<GameUI>();

    // Start playing
    setState(GameState::Playing);
}

void Game::continueGame() {
    std::cout << "Continuing game\n";

    // Load save data
    if (SaveSystem::load(saveData)) {
        levelHistory.clear();
    levelHistoryPos = -1;
        currentLevelPath.clear();
        currentLevelNumber = saveData.currentLevel;

        // Convert old save format (level number) to new format (level ID)
        // If currentLevelNumber <= 3, it's Zone 1
        std::string levelPath;
        if (currentLevelNumber == 1) {
            levelPath = "assets/levels/zone1_level1.json";
        } else if (currentLevelNumber == 2) {
            levelPath = "assets/levels/zone1_level2.json";
        } else if (currentLevelNumber == 3) {
            levelPath = "assets/levels/zone1_level3.json";
        } else if (currentLevelNumber == 4) {
            levelPath = "assets/levels/zone1_boss.json";
        } else {
            // Level number > 4: try legacy format (for future zones not yet converted)
            levelPath = "assets/levels/level" + std::to_string(currentLevelNumber) + ".json";
        }
        
        currentLevel = LevelLoader::loadFromFile(levelPath);
        
        // If loading failed, create default level
        if (!currentLevel || currentLevel->platforms.empty()) {
            std::cout << "Error: Could not load level " << currentLevelNumber << ". Creating default level.\n";
            currentLevel = LevelLoader::createDefaultLevel();
        }

        if (currentLevel) {
            platforms = std::move(currentLevel->platforms);
            checkpoints = std::move(currentLevel->checkpoints);
            interactiveObjects = std::move(currentLevel->interactiveObjects);
            goalZone = std::move(currentLevel->goalZone);

            // Determine spawn position: checkpoint if available, otherwise level start
            sf::Vector2f spawnPos = currentLevel->startPosition;
            if (saveData.activeCheckpointId[0] != '\0') {
                // Use saved checkpoint position
                spawnPos.x = saveData.checkpointX;
                spawnPos.y = saveData.checkpointY;
                activeCheckpointId = std::string(saveData.activeCheckpointId);

                // Activate the corresponding checkpoint in the level
                for (auto& checkpoint : checkpoints) {
                    if (checkpoint->getId() == activeCheckpointId) {
                        checkpoint->activate();
                        break;
                    }
                }

                std::cout << "Spawning at checkpoint: " << activeCheckpointId << "\n";
            } else {
                std::cout << "Spawning at level start\n";
            }

            // Create all 3 characters at spawn position
            auto lyra = std::make_unique<Player>(spawnPos.x, spawnPos.y, CharacterType::Lyra);
            lyra->setSpawnPoint(spawnPos.x, spawnPos.y);
            players.push_back(std::move(lyra));

            auto noah = std::make_unique<Player>(spawnPos.x, spawnPos.y, CharacterType::Noah);
            noah->setSpawnPoint(spawnPos.x, spawnPos.y);
            players.push_back(std::move(noah));

            auto sera = std::make_unique<Player>(spawnPos.x, spawnPos.y, CharacterType::Sera);
            sera->setSpawnPoint(spawnPos.x, spawnPos.y);
            players.push_back(std::move(sera));

            activePlayerIndex = 0;

            camera = std::make_unique<Camera>(
                static_cast<float>(Config::WINDOW_WIDTH),
                static_cast<float>(Config::WINDOW_HEIGHT)
            );

            if (!currentLevel->cameraZones.empty()) {
                const auto& zone = currentLevel->cameraZones[0];
                camera->setLimits(zone.minX, zone.maxX, zone.minY, zone.maxY);
            }

            gameUI = std::make_unique<GameUI>();

            if (!currentLevelPath.empty()) {
                levelHistory.push_back(currentLevelPath);
                levelHistoryPos = 0;
            }

            setState(GameState::Playing);
        }
    } else {
        // No save found, start new game
        startNewGame();
    }
}

void Game::switchCharacter() {
    if (players.size() < 2) return; // Need at least 2 characters to switch

    Player* currentPlayer = getActivePlayer();
    if (!currentPlayer) return;

    // Store current player position
    sf::Vector2f currentPos = currentPlayer->getPosition();
    sf::Vector2f currentVel = currentPlayer->getVelocity();
    bool wasGrounded = currentPlayer->getIsGrounded();

    // Switch to next character (cycle through)
    activePlayerIndex = (activePlayerIndex + 1) % players.size();

    // Transfer position and velocity to new character
    Player* newPlayer = getActivePlayer();
    if (newPlayer) {
        newPlayer->setPosition(currentPos.x, currentPos.y);
        newPlayer->setVelocity(currentVel.x, currentVel.y);
        newPlayer->setGrounded(wasGrounded);

        std::cout << "Switched to character: " << static_cast<int>(newPlayer->getCharacterType()) << "\n";
    }
}

void Game::returnToTitleScreen() {
    std::cout << "Returning to title screen\n";

    // Save current progress including checkpoint
    saveData.currentLevel = currentLevelNumber;

    // Save checkpoint position if one is active
    Player* player = getActivePlayer();
    if (!activeCheckpointId.empty() && player) {
        sf::Vector2f spawnPos = player->getSpawnPoint();
        saveData.checkpointX = spawnPos.x;
        saveData.checkpointY = spawnPos.y;
        // Copy checkpoint ID safely
        size_t len = activeCheckpointId.length();
        if (len > 63) len = 63;
        std::memcpy(saveData.activeCheckpointId, activeCheckpointId.c_str(), len);
        saveData.activeCheckpointId[len] = '\0';
    }

    SaveSystem::save(saveData);

    // Clean up game objects
    players.clear();
    platforms.clear();
    checkpoints.clear();
    interactiveObjects.clear();
    goalZone.reset();
    camera.reset();
    gameUI.reset();

    // Reset state
    levelCompleted = false;
    victoryEffectsTriggered = false;
    isTransitioning = false;
    activeCheckpointId = "";

    setState(GameState::TitleScreen);
}

void Game::setState(GameState newState) {
    if (newState != gameState) {
        previousState = gameState;
        gameState = newState;

        std::cout << "Game state changed to: " << static_cast<int>(gameState) << "\n";
    }
}
