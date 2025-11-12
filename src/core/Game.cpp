#include "core/Game.h"
#include "core/Config.h"
#include "core/InputConfig.h"
#include "physics/CollisionSystem.h"
#include <iostream>
#include <cstring>
#include <algorithm>

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
    , fpsUpdateTime(0.0f)
    , frameCount(0)
    , activeCheckpointId("")
{
    window.setFramerateLimit(Config::FRAMERATE_LIMIT);

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
}

void Game::run() {
    while (window.isOpen() && isRunning) {
        float dt = clock.restart().asSeconds();

        // Cap delta time to avoid spiral of death
        if (dt > 0.1f) dt = 0.1f;

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

    // Jump (check every frame for better responsiveness)
    if (sf::Keyboard::isKeyPressed(bindings.jump)) {
        player->jump();
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
    // Update menus based on state
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

    // Only update gameplay when playing or transitioning
    if (gameState != GameState::Playing && gameState != GameState::Transitioning) {
        return;
    }

    // Update screen transition
    if (isTransitioning) {
        screenTransition->update(dt);

        // When fade out completes, load the next level
        if (screenTransition->isFadedOut() && !nextLevelPath.empty()) {
            loadLevel(nextLevelPath);
            nextLevelPath = "";
            screenTransition->startFadeIn(0.5f);
        }

        // When fade in completes, resume gameplay
        if (screenTransition->isComplete()) {
            isTransitioning = false;
        }

        return; // Skip normal gameplay updates during transition
    }

    // Get active player
    Player* player = getActivePlayer();
    if (!player) {
        return; // No player yet (on title screen)
    }

    // Update ONLY the active player (inactive ones stay frozen)
    player->update(dt);

    // Check collisions with platforms FIRST
    sf::FloatRect playerBounds = player->getBounds();
    sf::Vector2f playerVel = player->getVelocity();
    bool grounded = false;

    for (auto& platform : platforms) {
        if (CollisionSystem::resolveCollision(
            playerBounds,
            playerVel,
            platform->getBounds(),
            grounded
        )) {
            player->setPosition(playerBounds.left, playerBounds.top);
            player->setVelocity(playerVel);
        }
    }

    player->setGrounded(grounded);

    // NOW check player events for effects (AFTER setGrounded sets the flags)
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

    // Clear event flags after processing them
    player->clearEventFlags();

    // Update UI health display
    gameUI->setHealth(player->getHealth(), player->getMaxHealth());

    // Track deaths
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
            player->setSpawnPoint(checkpoint->getSpawnPosition().x, checkpoint->getSpawnPosition().y);
            audioManager->playSound("checkpoint", 70.0f);

            // Visual feedback
            sf::Vector2f cpPos = checkpoint->getSpawnPosition();
            particleSystem->emitVictory(sf::Vector2f(cpPos.x + 20.0f, cpPos.y + 30.0f));
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
            sf::FloatRect playerBounds = player->getBounds();
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
                float tolerance = 10.0f * player->getStompDamageMultiplier();  // Noah has bigger stomp range
                bool playerFalling = player->getVelocity().y > 0;
                bool hitFromAbove = playerBounds.top + playerBounds.height <= enemyBounds.top + tolerance;

                if (playerFalling && hitFromAbove) {
                    // Kill enemy
                    enemy->kill();

                    // Bounce player up
                    player->setVelocity(player->getVelocity().x, -300.0f);

                    // Effects
                    sf::Vector2f enemyPos = enemy->getPosition();
                    particleSystem->emitDeath(sf::Vector2f(enemyPos.x + 15.0f, enemyPos.y + 15.0f));
                    audioManager->playSound("death", 80.0f);
                    cameraShake->shakeLight();
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

    // Update goal zone animation and emit particles
    if (goalZone) {
        goalZone->update(dt);
        sf::FloatRect goalBounds = goalZone->getBounds();
        particleSystem->emitGoalGlow(
            sf::Vector2f(goalBounds.left + goalBounds.width / 2.0f, goalBounds.top + goalBounds.height / 2.0f),
            sf::Vector2f(goalBounds.width, goalBounds.height)
        );

        // Check win condition
        if (!levelCompleted && goalZone->isPlayerInside(player->getBounds())) {
            levelCompleted = true;

            // Start transition to next level
            currentLevelNumber++;
            nextLevelPath = "assets/levels/level" + std::to_string(currentLevelNumber) + ".json";
            isTransitioning = true;
            screenTransition->startFadeOut(0.5f);
        }
    }

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
    window.clear(sf::Color(135, 206, 235)); // Sky blue background

    // Render menus
    if (gameState == GameState::TitleScreen && titleScreen) {
        titleScreen->draw(window);
        window.display();
        return;
    } else if (gameState == GameState::Paused && pauseMenu) {
        // Draw game in background (frozen)
        Player* player = getActivePlayer();
        if (camera && player) {
            camera->apply(window);
            for (auto& platform : platforms) platform->draw(window);
            for (auto& checkpoint : checkpoints) checkpoint->draw(window);
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
        // Draw pause menu on top
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

    // Render gameplay
    Player* player = getActivePlayer();
    if (gameState == GameState::Playing && camera && player) {
        // Apply camera
        camera->apply(window);

        // Draw platforms
        for (auto& platform : platforms) {
            platform->draw(window);
        }

        // Draw checkpoints
        for (auto& checkpoint : checkpoints) {
            checkpoint->draw(window);
        }

        // Draw goal zone
        if (goalZone) {
            goalZone->draw(window);
        }

        // Draw enemies
        for (auto& enemy : enemies) {
            if (enemy && enemy->isAlive()) {
                enemy->draw(window);
            }
        }

        // Draw particles (in world space)
        particleSystem->draw(window);

        // Draw only the active player
        if (player) {
            player->draw(window);
        }

        // Reset to default view for UI
        window.setView(window.getDefaultView());

        // Draw UI (deaths and timer)
        if (gameUI) {
            gameUI->draw(window);
        }

        // Draw FPS
        if (Config::SHOW_FPS && debugFont.getInfo().family != "") {
            window.draw(fpsText);
        }

        // Draw transition overlay (always on top)
        screenTransition->draw(window);
    }

    window.display();
}

void Game::loadLevel() {
    // Try to load level from file
    currentLevel = LevelLoader::loadFromFile("assets/levels/level1.json");

    if (currentLevel) {
        // Move data from LevelData to Game
        platforms = std::move(currentLevel->platforms);
        checkpoints = std::move(currentLevel->checkpoints);
        goalZone = std::move(currentLevel->goalZone);

        // Clear and create enemies (for now, add test enemies)
        enemies.clear();
        // Patrol enemy
        enemies.push_back(std::make_unique<PatrolEnemy>(200.0f, 520.0f, 80.0f));

        // Spike (stationary hazard)
        enemies.push_back(std::make_unique<Spike>(400.0f, 540.0f));
        enemies.push_back(std::make_unique<Spike>(440.0f, 540.0f));

        // Flying enemy (vertical patrol)
        enemies.push_back(std::make_unique<FlyingEnemy>(600.0f, 400.0f, 120.0f));

        std::cout << "Level loaded: " << currentLevel->name << "\n";
    }
}

void Game::loadLevel(const std::string& levelPath) {
    // Load level from specified path
    currentLevel = LevelLoader::loadFromFile(levelPath);

    if (currentLevel) {
        // Move data from LevelData to Game
        platforms = std::move(currentLevel->platforms);
        checkpoints = std::move(currentLevel->checkpoints);
        goalZone = std::move(currentLevel->goalZone);

        // Clear and create enemies (for now, add test enemies)
        enemies.clear();
        // Patrol enemy
        enemies.push_back(std::make_unique<PatrolEnemy>(200.0f, 520.0f, 80.0f));

        // Spike (stationary hazard)
        enemies.push_back(std::make_unique<Spike>(400.0f, 540.0f));
        enemies.push_back(std::make_unique<Spike>(440.0f, 540.0f));

        // Flying enemy (vertical patrol)
        enemies.push_back(std::make_unique<FlyingEnemy>(600.0f, 400.0f, 120.0f));

        // Reset all players to level start position
        sf::Vector2f startPos = currentLevel->startPosition;
        for (auto& p : players) {
            if (p) {
                p->setPosition(startPos.x, startPos.y);
                p->setSpawnPoint(startPos.x, startPos.y);
                p->setVelocity(sf::Vector2f(0.0f, 0.0f));
            }
        }

        // Reset level state
        levelCompleted = false;
        victoryEffectsTriggered = false;
        activeCheckpointId = "";

        // Update camera limits
        if (!currentLevel->cameraZones.empty()) {
            const auto& zone = currentLevel->cameraZones[0];
            camera->setLimits(zone.minX, zone.maxX, zone.minY, zone.maxY);
        }

        // Reset camera to active player position immediately (use update with 0 dt for instant snap)
        Player* activePlayer = getActivePlayer();
        if (activePlayer) {
            camera->update(activePlayer->getPosition(), 0.0f);
        }

        // Reset UI
        gameUI->hideVictoryMessage();

        std::cout << "Level loaded: " << currentLevel->name << "\n";
    }
}

void Game::startNewGame() {
    std::cout << "Starting new game\n";

    // Reset save data
    saveData = SaveData();
    saveData.currentLevel = 1;
    currentLevelNumber = 1;

    // Load level first
    loadLevel();

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
        currentLevelNumber = saveData.currentLevel;

        // Load saved level
        std::string levelPath = "assets/levels/level" + std::to_string(currentLevelNumber) + ".json";
        currentLevel = LevelLoader::loadFromFile(levelPath);

        if (currentLevel) {
            platforms = std::move(currentLevel->platforms);
            checkpoints = std::move(currentLevel->checkpoints);
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
        std::strncpy(saveData.activeCheckpointId, activeCheckpointId.c_str(), 63);
        saveData.activeCheckpointId[63] = '\0';
    }

    SaveSystem::save(saveData);

    // Clean up game objects
    players.clear();
    platforms.clear();
    checkpoints.clear();
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
