#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "entities/FlameTrap.h"

class Camera;
class Player;
class Platform;
class Enemy;
class InteractiveObject;
class Checkpoint;
struct LevelData;

namespace sf {
class RenderWindow;
}

struct EditorContext {
    sf::RenderWindow& window;
    Camera* camera;
    Player* activePlayer;
    std::vector<std::unique_ptr<Platform>>& platforms;
    std::vector<std::unique_ptr<Enemy>>& enemies;
    std::vector<std::unique_ptr<InteractiveObject>>& interactiveObjects;
    std::vector<std::unique_ptr<Checkpoint>>& checkpoints;
    LevelData* currentLevel;
    std::string& currentLevelPath;
    std::function<LevelData*(const std::string&)> reloadLevel;
};

class EditorController {
public:
    EditorController();

    void handleEvent(const sf::Event& event, EditorContext& ctx);
    void update(float dt, EditorContext& ctx);
    void render(EditorContext& ctx);
    void saveLevel(EditorContext& ctx);

    void resetState();

private:
    enum class ObjectType {
        Platform,
        PatrolEnemy,
        FlyingEnemy,
        Spike,
        FlameTrap,
        RotatingTrap,
        Terminal,
        Door,
        Turret,
        Checkpoint,
        Portal
    };

    ObjectType objectType = ObjectType::Platform;
    
    // Enemy preset selection
    enum class EnemyPresetType {
        Basic,
        Medium,
        Strong,
        Shooter,
        FastShooter,
        Boss,
        Fast,
        FlyingBasic,
        FlyingShooter,
        FlameHorizontal,
        FlameVertical,
        RotatingSlow,
        RotatingFast
    };
    EnemyPresetType currentEnemyPreset = EnemyPresetType::Basic;
    
    int selectedPlatformIndex = -1;
    int selectedEnemyIndex = -1;
    int selectedInteractiveIndex = -1;
    int selectedCheckpointIndex = -1;
    int selectedPortalIndex = -1;
    bool isDraggingPlatform = false;
    bool isDraggingEnemy = false;
    bool isDraggingInteractive = false;
    bool isDraggingCheckpoint = false;
    bool isDraggingPortal = false;
    sf::Vector2f dragOffset;

    sf::Font editorFont;
    sf::Text editorText;
    sf::Text saveMessageText;
    float saveMessageTimer = 0.0f;

    bool isFontLoaded() const;
    sf::Vector2f screenToWorld(const sf::Vector2f& screenPos, EditorContext& ctx) const;
    void setSaveMessage(const std::string& message, const sf::Color& color);
    void changeObjectType(ObjectType type);
    void reloadLevel(EditorContext& ctx);
    
    // Enemy preset helpers
    EnemyStats getPresetStats(EnemyPresetType preset) const;
    std::string getPresetName(EnemyPresetType preset) const;
    void applyPresetToEnemy(Enemy* enemy, EnemyPresetType preset, EditorContext& ctx);
    void applyPresetDefaults(EnemyPresetType preset);

    // Trap preset configuration
    FlameDirection currentFlameDirection = FlameDirection::Right;
    float currentFlameActive = 1.5f;
    float currentFlameInactive = 1.5f;
    float currentFlameInterval = 0.2f;
    float currentFlameProjectileSpeed = 350.0f;
    float currentFlameProjectileRange = 450.0f;

    float currentRotatingSpeed = 120.0f;
    float currentRotatingArmLength = 100.0f;
    float currentRotatingArmThickness = 16.0f;
};

