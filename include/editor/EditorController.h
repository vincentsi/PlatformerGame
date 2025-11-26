#pragma once

#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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
        Terminal,
        Door,
        Turret,
        Checkpoint,
        Portal
    };

    ObjectType objectType = ObjectType::Platform;
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
};

