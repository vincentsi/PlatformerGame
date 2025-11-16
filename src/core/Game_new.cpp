// Legacy MVP implementation of Game (single hardcoded level, no menus).
// Kept only as reference; main gameplay now lives in Game.cpp.
#include "core/Game.h"
#include "core/Config.h"
#include "physics/CollisionSystem.h"
#include <iostream>

Game::Game()
    : window(sf::VideoMode(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT), Config::WINDOW_TITLE)
    , isRunning(true)
    , playerWasDead(false)
    , levelCompleted(false)
    , victoryEffectsTriggered(false)
    , fpsUpdateTime(0.0f)
    , frameCount(0)
{
    window.setFramerateLimit(Config::FRAMERATE_LIMIT);

    // Create player
    player = std::make_unique<Player>(100.0f, 100.0f);

    // Create camera
    camera = std::make_unique<Camera>(
        static_cast<float>(Config::WINDOW_WIDTH),
        static_cast<float>(Config::WINDOW_HEIGHT)
    );

    // Create UI
    gameUI = std::make_unique<GameUI>();

    // Create polish systems
    particleSystem = std::make_unique<ParticleSystem>();
    cameraShake = std::make_unique<CameraShake>();
    audioManager = std::make_unique<AudioManager>();

    // Load audio files (optional - game works without them)
    audioManager->loadSound("jump", "assets/sounds/jump.wav");
    audioManager->loadSound("land", "assets/sounds/land.wav");
    audioManager->loadSound("death", "assets/sounds/death.wav");
    audioManager->loadSound("victory", "assets/sounds/victory.wav");
    audioManager->loadMusic("gameplay", "assets/music/gameplay.ogg");

    // Load level (hardcoded for MVP)
    loadLevel();

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
                window.close();
            }
        }
    }

    handleInput();
}

void Game::handleInput() {
    // Jump (check every frame for better responsiveness)
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        player->jump();
    }

    // Horizontal movement
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        player->moveLeft(1.0f / Config::FRAMERATE_LIMIT);
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        player->moveRight(1.0f / Config::FRAMERATE_LIMIT);
    }
    else {
        player->stopMoving();
    }
}

void Game::update(float dt) {
    // Update player
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

    // Update goal zone animation and emit particles
    goalZone->update(dt);
    sf::FloatRect goalBounds = goalZone->getBounds();
    particleSystem->emitGoalGlow(
        sf::Vector2f(goalBounds.left + goalBounds.width / 2.0f, goalBounds.top + goalBounds.height / 2.0f),
        sf::Vector2f(goalBounds.width, goalBounds.height)
    );

    // Check win condition
    if (!levelCompleted && goalZone->isPlayerInside(player->getBounds())) {
        levelCompleted = true;
        gameUI->showVictoryMessage();
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

    // Apply camera
    camera->apply(window);

    // Draw platforms
    for (auto& platform : platforms) {
        platform->draw(window);
    }

    // Draw goal zone
    goalZone->draw(window);

    // Draw particles (in world space)
    particleSystem->draw(window);

    // Draw player
    player->draw(window);

    // Reset to default view for UI
    window.setView(window.getDefaultView());

    // Draw UI (deaths and timer)
    gameUI->draw(window);

    // Draw FPS
    if (Config::SHOW_FPS && debugFont.getInfo().family != "") {
        window.draw(fpsText);
    }

    window.display();
}

void Game::loadLevel() {
    // MVP Level 1: Complete platformer challenge with goal

    // Starting area - Ground
    platforms.push_back(std::make_unique<Platform>(0.0f, 550.0f, 400.0f, 50.0f));

    // First jump section - ascending platforms
    platforms.push_back(std::make_unique<Platform>(500.0f, 500.0f, 150.0f, 20.0f));
    platforms.push_back(std::make_unique<Platform>(700.0f, 450.0f, 150.0f, 20.0f));
    platforms.push_back(std::make_unique<Platform>(900.0f, 400.0f, 150.0f, 20.0f));

    // High platform section
    platforms.push_back(std::make_unique<Platform>(1100.0f, 350.0f, 200.0f, 20.0f));
    platforms.push_back(std::make_unique<Platform>(1350.0f, 300.0f, 150.0f, 20.0f));

    // Gap jump challenge
    platforms.push_back(std::make_unique<Platform>(1600.0f, 300.0f, 150.0f, 20.0f));

    // Descending section
    platforms.push_back(std::make_unique<Platform>(1800.0f, 350.0f, 150.0f, 20.0f));
    platforms.push_back(std::make_unique<Platform>(2000.0f, 400.0f, 150.0f, 20.0f));

    // Final platform before goal
    platforms.push_back(std::make_unique<Platform>(2200.0f, 450.0f, 300.0f, 20.0f));

    // Goal zone at the end (golden area)
    goalZone = std::make_unique<GoalZone>(2400.0f, 370.0f, 80.0f, 80.0f);
}
