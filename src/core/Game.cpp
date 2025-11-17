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
#include <fstream>
#include <sstream>
#if __has_include(<nlohmann/json.hpp>)
    #include <nlohmann/json.hpp>
    #define GAME_HAS_JSON 1
#else
    #define GAME_HAS_JSON 0
#endif
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <cstdint>
#include <filesystem>

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
    Platform::initTilesets();

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

        if (!editorFont.loadFromFile("assets/fonts/arial.ttf")) {
            std::cout << "Warning: Could not load font for editor\n";
        } else {
            saveMessageText.setFont(editorFont);
            saveMessageText.setCharacterSize(24);
            saveMessageText.setFillColor(sf::Color::Green);
            saveMessageText.setPosition(Config::WINDOW_WIDTH / 2.0f - 150.0f, Config::WINDOW_HEIGHT / 2.0f - 20.0f);
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

            // Toggle editor mode with F1
            if (event.key.code == sf::Keyboard::F1) {
                if (gameState == GameState::Playing || gameState == GameState::Editor) {
                    setState(gameState == GameState::Editor ? GameState::Playing : GameState::Editor);
                }
            }
        }

        // Editor mode mouse handling
        if (gameState == GameState::Editor) {
            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
                sf::Vector2f worldPos = screenToWorld(sf::Vector2f(static_cast<float>(mousePixelPos.x), static_cast<float>(mousePixelPos.y)));

                if (event.mouseButton.button == sf::Mouse::Left) {
                    // Check if clicking on existing object
                    bool clickedObject = false;
                    
                    // Check platforms first
                    if (editorObjectType == EditorObjectType::Platform) {
                        for (size_t i = 0; i < platforms.size(); ++i) {
                            sf::FloatRect bounds = platforms[i]->getBounds();
                            if (bounds.contains(worldPos)) {
                                selectedPlatformIndex = static_cast<int>(i);
                                selectedEnemyIndex = -1;
                                isDraggingPlatform = true;
                                isDraggingEnemy = false;
                                dragOffset = worldPos - sf::Vector2f(bounds.left, bounds.top);
                                clickedObject = true;
                                break;
                            }
                        }
                        // If not clicking on platform, create new one
                        if (!clickedObject) {
                            platforms.push_back(std::make_unique<Platform>(worldPos.x, worldPos.y, 100.0f, 20.0f, Platform::Type::Floor));
                            selectedPlatformIndex = static_cast<int>(platforms.size() - 1);
                            selectedEnemyIndex = -1;
                            isDraggingPlatform = true;
                            isDraggingEnemy = false;
                            dragOffset = sf::Vector2f(0, 0);
                        }
                    } else {
                        // Check enemies
                        for (size_t i = 0; i < enemies.size(); ++i) {
                            sf::FloatRect bounds = enemies[i]->getBounds();
                            if (bounds.contains(worldPos)) {
                                selectedEnemyIndex = static_cast<int>(i);
                                selectedPlatformIndex = -1;
                                isDraggingEnemy = true;
                                isDraggingPlatform = false;
                                dragOffset = worldPos - sf::Vector2f(bounds.left, bounds.top);
                                clickedObject = true;
                                break;
                            }
                        }
                        // If not clicking on enemy, create new one based on editorObjectType
                        if (!clickedObject) {
                            switch (editorObjectType) {
                                case EditorObjectType::PatrolEnemy:
                                    enemies.push_back(std::make_unique<PatrolEnemy>(worldPos.x, worldPos.y, 100.0f));
                                    break;
                                case EditorObjectType::FlyingEnemy:
                                    enemies.push_back(std::make_unique<FlyingEnemy>(worldPos.x, worldPos.y, 200.0f, true));
                                    break;
                                case EditorObjectType::Spike:
                                    enemies.push_back(std::make_unique<Spike>(worldPos.x, worldPos.y));
                                    break;
                                default:
                                    break;
                            }
                            selectedEnemyIndex = static_cast<int>(enemies.size() - 1);
                            selectedPlatformIndex = -1;
                            isDraggingEnemy = true;
                            isDraggingPlatform = false;
                            dragOffset = sf::Vector2f(0, 0);
                        }
                    }
                } else if (event.mouseButton.button == sf::Mouse::Right) {
                    // Delete object under cursor
                    bool deleted = false;
                    // Check platforms
                    for (size_t i = 0; i < platforms.size(); ++i) {
                        sf::FloatRect bounds = platforms[i]->getBounds();
                        if (bounds.contains(worldPos)) {
                            platforms.erase(platforms.begin() + i);
                            if (selectedPlatformIndex == static_cast<int>(i)) {
                                selectedPlatformIndex = -1;
                            } else if (selectedPlatformIndex > static_cast<int>(i)) {
                                selectedPlatformIndex--;
                            }
                            deleted = true;
                            break;
                        }
                    }
                    // Check enemies if platform not deleted
                    if (!deleted) {
                        for (size_t i = 0; i < enemies.size(); ++i) {
                            sf::FloatRect bounds = enemies[i]->getBounds();
                            if (bounds.contains(worldPos)) {
                                enemies.erase(enemies.begin() + i);
                                if (selectedEnemyIndex == static_cast<int>(i)) {
                                    selectedEnemyIndex = -1;
                                } else if (selectedEnemyIndex > static_cast<int>(i)) {
                                    selectedEnemyIndex--;
                                }
                                break;
                            }
                        }
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    isDraggingPlatform = false;
                    isDraggingEnemy = false;
                }
            }
            
            // Change object type with number keys
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Num1) {
                    editorObjectType = EditorObjectType::Platform;
                    selectedEnemyIndex = -1;
                } else if (event.key.code == sf::Keyboard::Num2) {
                    editorObjectType = EditorObjectType::PatrolEnemy;
                    selectedPlatformIndex = -1;
                } else if (event.key.code == sf::Keyboard::Num3) {
                    editorObjectType = EditorObjectType::FlyingEnemy;
                    selectedPlatformIndex = -1;
                } else if (event.key.code == sf::Keyboard::Num4) {
                    editorObjectType = EditorObjectType::Spike;
                    selectedPlatformIndex = -1;
                }
            }

            // Save with Ctrl+S
            if (event.type == sf::Event::KeyPressed && 
                event.key.code == sf::Keyboard::S && 
                sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
                saveLevelToFile();
            }

            // Delete selected object with Delete key
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Delete) {
                if (selectedPlatformIndex >= 0 && selectedPlatformIndex < static_cast<int>(platforms.size())) {
                    platforms.erase(platforms.begin() + selectedPlatformIndex);
                    selectedPlatformIndex = -1;
                } else if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(enemies.size())) {
                    enemies.erase(enemies.begin() + selectedEnemyIndex);
                    selectedEnemyIndex = -1;
                }
            }
            // Reload level from file (F5)
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F5) {
                if (!currentLevelPath.empty()) {
                    loadLevel(currentLevelPath);
                    selectedPlatformIndex = -1;
                    selectedEnemyIndex = -1;
                    isDraggingPlatform = false;
                    isDraggingEnemy = false;
                }
            }
            // Change platform type with T key
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::T) {
                if (selectedPlatformIndex >= 0 && selectedPlatformIndex < static_cast<int>(platforms.size())) {
                    // Cycle through platform types
                    Platform::Type currentType = platforms[selectedPlatformIndex]->getType();
                    Platform::Type nextType;
                    switch (currentType) {
                        case Platform::Type::Floor: nextType = Platform::Type::EndFloor; break;
                        case Platform::Type::EndFloor: nextType = Platform::Type::Floor; break;
                        default: nextType = Platform::Type::Floor; break;
                    }
                    platforms[selectedPlatformIndex]->setType(nextType);
                    std::cout << "Type de plateforme change\n";
                }
            }
            
            // Resize platform with +/- keys
            if (event.type == sf::Event::KeyPressed && selectedPlatformIndex >= 0 && selectedPlatformIndex < static_cast<int>(platforms.size())) {
                sf::Vector2f currentSize = platforms[selectedPlatformIndex]->getSize();
                float resizeStep = 10.0f;
                
                if (event.key.code == sf::Keyboard::Add || event.key.code == sf::Keyboard::Equal) {
                    // Increase width
                    platforms[selectedPlatformIndex]->setSize(currentSize.x + resizeStep, currentSize.y);
                } else if (event.key.code == sf::Keyboard::Subtract || event.key.code == sf::Keyboard::Hyphen) {
                    // Decrease width
                    platforms[selectedPlatformIndex]->setSize(std::max(10.0f, currentSize.x - resizeStep), currentSize.y);
                } else if (event.key.code == sf::Keyboard::PageUp) {
                    // Increase height
                    platforms[selectedPlatformIndex]->setSize(currentSize.x, currentSize.y + resizeStep);
                } else if (event.key.code == sf::Keyboard::PageDown) {
                    // Decrease height
                    platforms[selectedPlatformIndex]->setSize(currentSize.x, std::max(10.0f, currentSize.y - resizeStep));
                }
            }
            
            // Modify enemy patrol distance with Q/W keys
            if (event.type == sf::Event::KeyPressed && selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(enemies.size())) {
                Enemy* enemy = enemies[selectedEnemyIndex].get();
                float currentDistance = enemy->getPatrolDistance();
                float distanceStep = 20.0f;
                
                if (event.key.code == sf::Keyboard::Q) {
                    // Decrease patrol distance
                    enemy->setPatrolDistance(std::max(20.0f, currentDistance - distanceStep));
                } else if (event.key.code == sf::Keyboard::W) {
                    // Increase patrol distance
                    enemy->setPatrolDistance(currentDistance + distanceStep);
                }
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
    } else if (gameState == GameState::Editor) {
        updateEditor(dt);
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
    } else if (gameState == GameState::Editor) {
        renderEditor();
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

            // Load enemies from JSON
            enemies = std::move(currentLevel->enemies);
            
        kineticWaveProjectiles.clear();

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

sf::Vector2f Game::screenToWorld(const sf::Vector2f& screenPos) {
    if (!camera) return screenPos;
    
    sf::View view = window.getView();
    sf::Vector2f worldPos = window.mapPixelToCoords(sf::Vector2i(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y)), view);
    return worldPos;
}

void Game::updateEditor(float dt) {
    if (!camera) return;

    // Update save message timer
    if (saveMessageTimer > 0.0f) {
        saveMessageTimer -= dt;
    }

    Player* player = getActivePlayer();
    if (player) {
        camera->update(player->getPosition(), dt);
    }

    // Handle dragging
    if (isDraggingPlatform && selectedPlatformIndex >= 0 && selectedPlatformIndex < static_cast<int>(platforms.size())) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = screenToWorld(sf::Vector2f(static_cast<float>(mousePixelPos.x), static_cast<float>(mousePixelPos.y)));
        
        sf::FloatRect bounds = platforms[selectedPlatformIndex]->getBounds();
        platforms[selectedPlatformIndex]->setPosition(worldPos.x - dragOffset.x, worldPos.y - dragOffset.y);
    }
    
    if (isDraggingEnemy && selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(enemies.size())) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = screenToWorld(sf::Vector2f(static_cast<float>(mousePixelPos.x), static_cast<float>(mousePixelPos.y)));
        
        Enemy* enemy = enemies[selectedEnemyIndex].get();
        sf::Vector2f oldPos = enemy->getPosition();
        enemy->setPosition(worldPos.x - dragOffset.x, worldPos.y - dragOffset.y);
        
        // Update patrol bounds to follow the enemy position
        sf::Vector2f newPos = enemy->getPosition();
        float distanceX = newPos.x - oldPos.x;
        float currentDistance = enemy->getPatrolDistance();
        float centerX = (enemy->getLeftBound() + enemy->getRightBound()) / 2.0f;
        float newCenterX = centerX + distanceX;
        enemy->setPatrolBounds(newCenterX - currentDistance / 2.0f, newCenterX + currentDistance / 2.0f);
    }

    // Camera movement with arrow keys (move player to move camera)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        if (player) {
            player->setPosition(player->getPosition().x - 500.0f * dt, player->getPosition().y);
        }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        if (player) {
            player->setPosition(player->getPosition().x + 500.0f * dt, player->getPosition().y);
        }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        if (player) {
            player->setPosition(player->getPosition().x, player->getPosition().y - 500.0f * dt);
        }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        if (player) {
            player->setPosition(player->getPosition().x, player->getPosition().y + 500.0f * dt);
        }
    }
}

void Game::renderEditor() {
    if (!camera) return;

    Player* player = getActivePlayer();
    if (player) {
        camera->apply(window);
    }

    drawParallaxBackground(window);

    // Draw platforms
    for (size_t i = 0; i < platforms.size(); ++i) {
        platforms[i]->draw(window);
        
        // Highlight selected platform
        if (static_cast<int>(i) == selectedPlatformIndex) {
            sf::FloatRect bounds = platforms[i]->getBounds();
            sf::RectangleShape outline;
            outline.setSize(sf::Vector2f(bounds.width, bounds.height));
            outline.setPosition(bounds.left, bounds.top);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::Yellow);
            outline.setOutlineThickness(2.0f);
            window.draw(outline);
        }

        // Draw platform index
        if (editorFont.getInfo().family != "") {
            sf::Text indexText;
            indexText.setFont(editorFont);
            indexText.setString(std::to_string(i));
            indexText.setCharacterSize(12);
            indexText.setFillColor(sf::Color::White);
            sf::FloatRect bounds = platforms[i]->getBounds();
            indexText.setPosition(bounds.left + bounds.width / 2.0f - 10.0f, bounds.top - 15.0f);
            window.draw(indexText);
        }
    }
    
    // Draw enemies
    for (size_t i = 0; i < enemies.size(); ++i) {
        enemies[i]->draw(window);
        
        Enemy* enemy = enemies[i].get();
        
        // Draw patrol zone for patrol and flying enemies
        if (enemy->getType() == EnemyType::Patrol || enemy->getType() == EnemyType::Flying) {
            float leftBound = enemy->getLeftBound();
            float rightBound = enemy->getRightBound();
            sf::Vector2f pos = enemy->getPosition();
            
            // Draw patrol zone as a line
            sf::RectangleShape patrolLine;
            patrolLine.setSize(sf::Vector2f(rightBound - leftBound, 2.0f));
            patrolLine.setPosition(leftBound, pos.y - 40.0f);
            patrolLine.setFillColor(sf::Color(255, 255, 0, 100)); // Yellow, semi-transparent
            window.draw(patrolLine);
            
            // Draw bounds markers
            sf::CircleShape leftMarker(3.0f);
            leftMarker.setPosition(leftBound - 3.0f, pos.y - 41.0f);
            leftMarker.setFillColor(sf::Color::Yellow);
            window.draw(leftMarker);
            
            sf::CircleShape rightMarker(3.0f);
            rightMarker.setPosition(rightBound - 3.0f, pos.y - 41.0f);
            rightMarker.setFillColor(sf::Color::Yellow);
            window.draw(rightMarker);
        }
        
        // Highlight selected enemy
        if (static_cast<int>(i) == selectedEnemyIndex) {
            sf::FloatRect bounds = enemies[i]->getBounds();
            sf::RectangleShape outline;
            outline.setSize(sf::Vector2f(bounds.width, bounds.height));
            outline.setPosition(bounds.left, bounds.top);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::Cyan);
            outline.setOutlineThickness(2.0f);
            window.draw(outline);
        }
        
        // Draw enemy index and patrol distance
        if (editorFont.getInfo().family != "") {
            sf::Text indexText;
            indexText.setFont(editorFont);
            std::string info = "E" + std::to_string(i);
            if (enemy->getType() == EnemyType::Patrol || enemy->getType() == EnemyType::Flying) {
                info += " (" + std::to_string(static_cast<int>(enemy->getPatrolDistance())) + ")";
            }
            indexText.setString(info);
            indexText.setCharacterSize(12);
            indexText.setFillColor(sf::Color::Cyan);
            sf::FloatRect bounds = enemies[i]->getBounds();
            indexText.setPosition(bounds.left + bounds.width / 2.0f - 20.0f, bounds.top - 15.0f);
            window.draw(indexText);
        }
    }

    // Draw checkpoints
    for (auto& checkpoint : checkpoints) {
        checkpoint->draw(window);
    }

    // Draw goal zone
    if (goalZone) {
        goalZone->draw(window);
    }

    window.setView(window.getDefaultView());

    // Draw editor UI
    if (editorFont.getInfo().family != "") {
        editorText.setFont(editorFont);
        editorText.setCharacterSize(16);
        editorText.setFillColor(sf::Color::Black);
        editorText.setPosition(10.0f, 10.0f);
        
        std::string objectTypeStr = "Platform";
        switch (editorObjectType) {
            case EditorObjectType::Platform: objectTypeStr = "Platform"; break;
            case EditorObjectType::PatrolEnemy: objectTypeStr = "PatrolEnemy"; break;
            case EditorObjectType::FlyingEnemy: objectTypeStr = "FlyingEnemy"; break;
            case EditorObjectType::Spike: objectTypeStr = "Spike"; break;
        }
        
        std::string info = "MODE EDITEUR\n";
        info += "F1: Toggle Editor\n";
        info += "1-4: Type objet (" + objectTypeStr + ")\n";
        info += "Clic Gauche: Placer/Selectionner\n";
        info += "Clic Droit: Supprimer\n";
        info += "Delete: Supprimer selectionnee\n";
        info += "Ctrl+S: Sauvegarder\n";
        info += "F5: Recharger depuis fichier\n";
        info += "Fleches: Deplacer camera\n";
        info += "+/-: Largeur plateforme\n";
        info += "PageUp/Down: Hauteur plateforme\n";
        info += "T: Changer type plateforme\n";
        if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(enemies.size())) {
            Enemy* enemy = enemies[selectedEnemyIndex].get();
            if (enemy->getType() == EnemyType::Patrol || enemy->getType() == EnemyType::Flying) {
                info += "Q/W: Distance patrouille (" + std::to_string(static_cast<int>(enemy->getPatrolDistance())) + ")\n";
            }
        }
        info += "Plateformes: " + std::to_string(platforms.size()) + "\n";
        info += "Ennemis: " + std::to_string(enemies.size());
        
        editorText.setString(info);
        window.draw(editorText);
    }

    // Draw save confirmation message
    if (saveMessageTimer > 0.0f && editorFont.getInfo().family != "") {
        float alpha = std::min(255.0f, saveMessageTimer * 255.0f / 2.0f);
        saveMessageText.setFillColor(sf::Color(saveMessageText.getFillColor().r, 
                                               saveMessageText.getFillColor().g, 
                                               saveMessageText.getFillColor().b, 
                                               static_cast<sf::Uint8>(alpha)));
        window.draw(saveMessageText);
    }
}

void Game::saveLevelToFile() {
    if (currentLevelPath.empty()) {
        std::cout << "Erreur: Pas de niveau charge pour sauvegarder\n";
        return;
    }

    // Always save to source directory: PlatformerGame/assets/levels/...
    // Extract filename from currentLevelPath (e.g., "zone1_level1.json")
    std::string filename;
    size_t lastSlash = currentLevelPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = currentLevelPath.substr(lastSlash + 1);
    } else {
        filename = currentLevelPath;
    }
    
    // Try to find the source directory by going up from current working directory
    // We're likely in build/bin/Release or similar, need to go up to nouveauprojet/PlatformerGame/
    std::string savePath;
    
    try {
        // Get current working directory
        std::filesystem::path currentDir = std::filesystem::current_path();
        std::filesystem::path sourcePath = currentDir;
        
        // Try to find "nouveauprojet" or "PlatformerGame" in the path
        bool foundNouveauprojet = false;
        for (int i = 0; i < 10; ++i) {
            if (sourcePath.filename() == "nouveauprojet" || sourcePath.filename() == "PlatformerGame") {
                foundNouveauprojet = true;
                break;
            }
            if (sourcePath.has_parent_path()) {
                sourcePath = sourcePath.parent_path();
            } else {
                break;
            }
        }
        
        if (foundNouveauprojet) {
            if (sourcePath.filename() == "nouveauprojet") {
                savePath = (sourcePath / "PlatformerGame" / "assets" / "levels" / filename).string();
            } else {
                savePath = (sourcePath / "assets" / "levels" / filename).string();
            }
            std::cout << "Chemin source construit: " << savePath << "\n";
        } else {
            // Fallback: try relative paths
            std::vector<std::string> possiblePaths = {
                "../../../PlatformerGame/assets/levels/" + filename,
                "../../PlatformerGame/assets/levels/" + filename,
                "../PlatformerGame/assets/levels/" + filename,
                "PlatformerGame/assets/levels/" + filename
            };
            
            bool found = false;
            for (const auto& path : possiblePaths) {
                std::ifstream test(path);
                if (test.is_open()) {
                    test.close();
                    savePath = path;
                    found = true;
                    std::cout << "Fichier source trouve (relatif): " << savePath << "\n";
                    break;
                }
            }
            
            if (!found) {
                savePath = "../../../PlatformerGame/assets/levels/" + filename;
                std::cout << "Utilisation du chemin par defaut: " << savePath << "\n";
            }
        }
    } catch (const std::exception&) {
        // Fallback to relative path if filesystem operations fail
        savePath = "../../../PlatformerGame/assets/levels/" + filename;
        std::cout << "Erreur filesystem, utilisation chemin par defaut: " << savePath << "\n";
    }

#if GAME_HAS_JSON
    // Load existing JSON structure
    std::ifstream file(savePath);
    if (!file.is_open()) {
        std::cout << "Erreur: Impossible d'ouvrir " << savePath << " pour sauvegarde\n";
        std::cout << "Tentative avec le chemin original: " << currentLevelPath << "\n";
        saveMessageTimer = 2.0f;
        saveMessageText.setString("Erreur: Fichier introuvable");
        saveMessageText.setFillColor(sf::Color::Red);
        return;
    }

    nlohmann::json j;
    file >> j;
    file.close();

    // Update platforms array
    j["platforms"] = nlohmann::json::array();
    for (size_t i = 0; i < platforms.size(); ++i) {
        const auto& platform = platforms[i];
        sf::FloatRect bounds = platform->getBounds();
        nlohmann::json p;
        p["x"] = bounds.left;
        p["y"] = bounds.top;
        p["width"] = bounds.width;
        p["height"] = bounds.height;
        // Save platform type
        std::string typeStr = "floor";
        Platform::Type currentType = platform->getType();
        switch (currentType) {
            case Platform::Type::Floor: typeStr = "floor"; break;
            case Platform::Type::EndFloor: typeStr = "endfloor"; break;
        }
        p["type"] = typeStr;
        j["platforms"].push_back(p);
        std::cout << "Plateforme " << i << " sauvegardee avec type: " << typeStr << "\n";
    }
    
    // Update enemies array
    j["enemies"] = nlohmann::json::array();
    for (size_t i = 0; i < enemies.size(); ++i) {
        const auto& enemy = enemies[i];
        sf::Vector2f pos = enemy->getPosition();
        nlohmann::json e;
        e["x"] = pos.x;
        e["y"] = pos.y;
        
        // Determine enemy type using dynamic_cast
        if (dynamic_cast<PatrolEnemy*>(enemy.get())) {
            e["type"] = "patrol";
            e["patrolDistance"] = enemy->getPatrolDistance();
        } else if (dynamic_cast<FlyingEnemy*>(enemy.get())) {
            e["type"] = "flying";
            e["patrolDistance"] = enemy->getPatrolDistance();
            e["horizontalPatrol"] = true;
        } else if (dynamic_cast<Spike*>(enemy.get())) {
            e["type"] = "spike";
        }
        j["enemies"].push_back(e);
        std::cout << "Ennemi " << i << " sauvegarde avec type: " << e["type"].get<std::string>() << "\n";
    }

    // Save back to file
    std::ofstream outFile(savePath);
    if (!outFile.is_open()) {
        std::cout << "Erreur: Impossible d'ecrire dans " << savePath << "\n";
        std::cout << "Chemin original: " << currentLevelPath << "\n";
        return;
    }

    outFile << j.dump(2);
    outFile.close();

    std::cout << "Niveau sauvegarde dans " << savePath << " (" << platforms.size() << " plateformes)\n";
    std::cout << "Verifie le fichier: " << savePath << "\n";
    
    // Don't reload automatically - the platforms are already in memory with correct types
    // Reloading would use savePath which might not work correctly
    // User can manually reload with F5 if needed
    selectedPlatformIndex = -1;
    isDraggingPlatform = false;
    
    // Show save confirmation message
    saveMessageTimer = 2.0f;
    saveMessageText.setString("Niveau sauvegarde !");
    saveMessageText.setFillColor(sf::Color::Green);
#else
    // Fallback: manual JSON writing
    std::ifstream inFile(savePath);
    if (!inFile.is_open()) {
        std::cout << "Erreur: Impossible d'ouvrir " << savePath << " pour sauvegarde\n";
        saveMessageTimer = 2.0f;
        saveMessageText.setString("Erreur: Fichier introuvable");
        saveMessageText.setFillColor(sf::Color::Red);
        return;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string content = buffer.str();
    inFile.close();
    
    // Update savePath if we used fallback
    if (savePath != currentLevelPath && savePath.find("PlatformerGame/") == std::string::npos) {
        // Re-check if we need to convert path
        size_t buildPos = savePath.find("build/");
        if (buildPos != std::string::npos) {
            size_t assetsPos = savePath.find("assets/");
            if (assetsPos != std::string::npos) {
                savePath = "PlatformerGame/" + savePath.substr(assetsPos);
            }
        }
    }

    // Find platforms section and replace it
    size_t platformsStart = content.find("\"platforms\"");
    if (platformsStart == std::string::npos) {
        std::cout << "Erreur: Section platforms non trouvee dans le JSON\n";
        saveMessageTimer = 2.0f;
        saveMessageText.setString("Erreur: Format JSON invalide");
        saveMessageText.setFillColor(sf::Color::Red);
        return;
    }

    size_t arrayStart = content.find("[", platformsStart);
    if (arrayStart == std::string::npos) {
        std::cout << "Erreur: Tableau platforms non trouve\n";
        saveMessageTimer = 2.0f;
        saveMessageText.setString("Erreur: Format JSON invalide");
        saveMessageText.setFillColor(sf::Color::Red);
        return;
    }

    size_t arrayEnd = arrayStart + 1;
    int bracketDepth = 1;
    while (arrayEnd < content.length() && bracketDepth > 0) {
        if (content[arrayEnd] == '[') bracketDepth++;
        else if (content[arrayEnd] == ']') bracketDepth--;
        arrayEnd++;
    }

    // Build new platforms array
    std::string newPlatforms = "[\n";
    for (size_t i = 0; i < platforms.size(); ++i) {
        sf::FloatRect bounds = platforms[i]->getBounds();
        Platform::Type currentType = platforms[i]->getType();
        std::string typeStr = (currentType == Platform::Type::EndFloor) ? "endfloor" : "floor";
        
        newPlatforms += "    { \"x\": " + std::to_string(static_cast<int>(bounds.left)) + 
                       ", \"y\": " + std::to_string(static_cast<int>(bounds.top)) +
                       ", \"width\": " + std::to_string(static_cast<int>(bounds.width)) +
                       ", \"height\": " + std::to_string(static_cast<int>(bounds.height)) +
                       ", \"type\": \"" + typeStr + "\" }";
        if (i < platforms.size() - 1) {
            newPlatforms += ",";
        }
        newPlatforms += "\n";
        std::cout << "Plateforme " << i << " sauvegardee (fallback) avec type: " << typeStr << "\n";
    }
    newPlatforms += "  ]";

    // Replace platforms array in content
    content.replace(arrayStart, arrayEnd - arrayStart, newPlatforms);
    
    // Also update enemies array if it exists
    size_t enemiesStart = content.find("\"enemies\"");
    if (enemiesStart != std::string::npos) {
        size_t enemiesArrayStart = content.find("[", enemiesStart);
        if (enemiesArrayStart != std::string::npos) {
            size_t enemiesArrayEnd = enemiesArrayStart + 1;
            int enemiesBracketDepth = 1;
            while (enemiesArrayEnd < content.length() && enemiesBracketDepth > 0) {
                if (content[enemiesArrayEnd] == '[') enemiesBracketDepth++;
                else if (content[enemiesArrayEnd] == ']') enemiesBracketDepth--;
                enemiesArrayEnd++;
            }
            
            std::string newEnemies = "[\n";
            for (size_t i = 0; i < enemies.size(); ++i) {
                sf::Vector2f pos = enemies[i]->getPosition();
                std::string typeStr = "patrol";
                if (dynamic_cast<FlyingEnemy*>(enemies[i].get())) {
                    typeStr = "flying";
                } else if (dynamic_cast<Spike*>(enemies[i].get())) {
                    typeStr = "spike";
                }
                
                newEnemies += "    { \"type\": \"" + typeStr + "\", \"x\": " + std::to_string(static_cast<int>(pos.x)) + 
                             ", \"y\": " + std::to_string(static_cast<int>(pos.y));
                if (typeStr == "patrol" || typeStr == "flying") {
                    float patrolDist = enemies[i]->getPatrolDistance();
                    newEnemies += ", \"patrolDistance\": " + std::to_string(static_cast<int>(patrolDist));
                }
                if (typeStr == "flying") {
                    newEnemies += ", \"horizontalPatrol\": true";
                }
                newEnemies += " }";
                if (i < enemies.size() - 1) {
                    newEnemies += ",";
                }
                newEnemies += "\n";
            }
            newEnemies += "  ]";
            
            content.replace(enemiesArrayStart, enemiesArrayEnd - enemiesArrayStart, newEnemies);
        }
    } else {
        // Insert enemies array after platforms
        size_t insertPos = content.find_last_of("]");
        if (insertPos != std::string::npos) {
            std::string newEnemies = ",\n  \"enemies\": [\n";
            for (size_t i = 0; i < enemies.size(); ++i) {
                sf::Vector2f pos = enemies[i]->getPosition();
                std::string typeStr = "patrol";
                if (dynamic_cast<FlyingEnemy*>(enemies[i].get())) {
                    typeStr = "flying";
                } else if (dynamic_cast<Spike*>(enemies[i].get())) {
                    typeStr = "spike";
                }
                
                newEnemies += "    { \"type\": \"" + typeStr + "\", \"x\": " + std::to_string(static_cast<int>(pos.x)) + 
                             ", \"y\": " + std::to_string(static_cast<int>(pos.y));
                if (typeStr == "patrol" || typeStr == "flying") {
                    float patrolDist = enemies[i]->getPatrolDistance();
                    newEnemies += ", \"patrolDistance\": " + std::to_string(static_cast<int>(patrolDist));
                }
                if (typeStr == "flying") {
                    newEnemies += ", \"horizontalPatrol\": true";
                }
                newEnemies += " }";
                if (i < enemies.size() - 1) {
                    newEnemies += ",";
                }
                newEnemies += "\n";
            }
            newEnemies += "  ]";
            content.insert(insertPos + 1, newEnemies);
        }
    }

    // Write back to file
    std::ofstream outFile(savePath);
    if (!outFile.is_open()) {
        std::cout << "Erreur: Impossible d'ecrire dans " << savePath << "\n";
        saveMessageTimer = 2.0f;
        saveMessageText.setString("Erreur: Ecriture impossible");
        saveMessageText.setFillColor(sf::Color::Red);
        return;
    }

    outFile << content;
    outFile.close();

    std::cout << "Niveau sauvegarde (fallback) dans " << savePath << " (" << platforms.size() << " plateformes)\n";
    std::cout << "Verifie le fichier: " << savePath << "\n";
    
    // Don't reload automatically - the platforms are already in memory with correct types
    // Reloading would use savePath which might not work correctly
    // User can manually reload with F5 if needed
    selectedPlatformIndex = -1;
    isDraggingPlatform = false;
    
    // Show save confirmation message
    saveMessageTimer = 2.0f;
    saveMessageText.setString("Niveau sauvegarde !");
    saveMessageText.setFillColor(sf::Color::Green);
#endif
}
