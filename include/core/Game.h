#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "entities/PatrolEnemy.h"
#include "entities/Spike.h"
#include "entities/FlyingEnemy.h"
#include "world/Platform.h"
#include "world/Camera.h"
#include "world/GoalZone.h"
#include "world/Checkpoint.h"
#include "world/InteractiveObject.h"
#include "world/LevelLoader.h"
#include "ui/GameUI.h"
#include "effects/ParticleSystem.h"
#include "effects/CameraShake.h"
#include "effects/ScreenTransition.h"
#include "audio/AudioManager.h"
#include "core/GameState.h"
#include "core/SaveSystem.h"
#include "ui/TitleScreen.h"
#include "ui/PauseMenu.h"
#include "ui/SettingsMenu.h"
#include "ui/KeyBindingMenu.h"

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void processEvents();
    void update(float dt);
    void render();

    void handleInput();
    void loadLevel();
    void loadLevel(const std::string& levelPath);

    // Helper to get active player
    Player* getActivePlayer() { return activePlayerIndex < players.size() ? players[activePlayerIndex].get() : nullptr; }
    void switchCharacter();

    // Menu actions
    void startNewGame();
    void continueGame();
    void returnToTitleScreen();
    void setState(GameState newState);

private:
    sf::RenderWindow window;
    sf::Clock clock;

    std::vector<std::unique_ptr<Player>> players;
    int activePlayerIndex;
    std::vector<std::unique_ptr<Platform>> platforms;
    std::vector<std::unique_ptr<Checkpoint>> checkpoints;
    std::vector<std::unique_ptr<InteractiveObject>> interactiveObjects;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<GameUI> gameUI;
    std::unique_ptr<GoalZone> goalZone;

    // Level system
    std::unique_ptr<LevelData> currentLevel;
    std::string activeCheckpointId;

    // Polish systems
    std::unique_ptr<ParticleSystem> particleSystem;
    std::unique_ptr<CameraShake> cameraShake;
    std::unique_ptr<AudioManager> audioManager;
    std::unique_ptr<ScreenTransition> screenTransition;

    // State management
    GameState gameState;
    GameState previousState;
    SaveData saveData;

    bool isRunning;
    bool playerWasDead;
    bool levelCompleted;
    bool victoryEffectsTriggered;
    bool isTransitioning;
    std::string nextLevelPath;
    int currentLevelNumber;
    bool secretRoomUnlocked;

    // Menus
    std::unique_ptr<TitleScreen> titleScreen;
    std::unique_ptr<PauseMenu> pauseMenu;
    std::unique_ptr<SettingsMenu> settingsMenu;
    std::unique_ptr<KeyBindingMenu> keyBindingMenu;

    // Debug
    sf::Font debugFont;
    sf::Text fpsText;
    float fpsUpdateTime;
    int frameCount;
};
