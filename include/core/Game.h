#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/GameState.h"
#include "core/SaveSystem.h"

// Forward declarations to reduce compile-time coupling
class Player;
class Enemy;
class KineticWaveProjectile;
class Platform;
class Camera;
class GoalZone;
class Checkpoint;
class InteractiveObject;
struct LevelData;
class GameUI;
class ParticleSystem;
class CameraShake;
class ScreenTransition;
class AudioManager;
class TitleScreen;
class PauseMenu;
class SettingsMenu;
class KeyBindingMenu;

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
    void goBackOneLevel();
    
    // Background rendering
    void drawParallaxBackground(sf::RenderWindow& window);

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
    std::vector<std::unique_ptr<KineticWaveProjectile>> kineticWaveProjectiles;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<GameUI> gameUI;
    std::unique_ptr<GoalZone> goalZone;

    // Level system
    std::unique_ptr<LevelData> currentLevel;
    std::string currentLevelPath;
    std::string activeCheckpointId;
    std::vector<std::string> levelHistory;
    int levelHistoryPos = -1;
    std::unordered_map<std::string, std::string> levelCheckpoints;
    int pendingSpawnEdge = -1;

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
    int postTransitionHideFrames = 0;
    std::string nextLevelPath;
    int currentLevelNumber;
    bool secretRoomUnlocked;

    // Menus
    std::unique_ptr<TitleScreen> titleScreen;
    std::unique_ptr<PauseMenu> pauseMenu;
    std::unique_ptr<SettingsMenu> settingsMenu;
    std::unique_ptr<KeyBindingMenu> keyBindingMenu;

    // Background walls
    sf::Texture* bgWallPlain32;
    sf::Texture* bgWallCables32;
    sf::Texture* bgFarTexture;
    sf::Texture* bgWallPlainVarA32;
    sf::Texture* bgWallPlainVarB32;
    sf::Texture* bgWallCablesAlt32;
    
    // Debug
    sf::Font debugFont;
    sf::Text fpsText;
    float fpsUpdateTime;
    int frameCount;

    // Per-frame input / ability state that used to be static locals in update()
    bool doorKeyHeld = false;
    float lastAbilityTimer = 0.0f;

    // Editor mode
    int selectedPlatformIndex = -1;
    bool isDraggingPlatform = false;
    sf::Vector2f dragOffset;
    sf::Font editorFont;
    sf::Text editorText;
    sf::Text saveMessageText;
    float saveMessageTimer = 0.0f;
    void handleEditorInput();
    void updateEditor(float dt);
    void renderEditor();
    void saveLevelToFile();
    sf::Vector2f screenToWorld(const sf::Vector2f& screenPos);
};
