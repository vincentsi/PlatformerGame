#include "editor/EditorController.h"
#include "entities/Enemy.h"
#include "entities/FlyingEnemy.h"
#include "entities/PatrolEnemy.h"
#include "entities/FlameTrap.h"
#include "entities/RotatingTrap.h"
#include "entities/EnemyStatsPresets.h"
#include "entities/Player.h"
#include "entities/Spike.h"
#include "world/Checkpoint.h"
#include <cctype>
#include "world/Camera.h"
#include "world/InteractiveObject.h"
#include "world/LevelLoader.h"
#include "world/Platform.h"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#if __has_include(<nlohmann/json.hpp>)
    #define GAME_HAS_JSON 1
#else
    #define GAME_HAS_JSON 0
#endif

#if GAME_HAS_JSON
#include <nlohmann/json.hpp>
#endif

EditorController::EditorController() {
    if (!editorFont.loadFromFile("assets/fonts/arial.ttf")) {
        std::cout << "Warning: Unable to load editor font assets/fonts/arial.ttf\n";
    }

    if (isFontLoaded()) {
        editorText.setFont(editorFont);
        saveMessageText.setFont(editorFont);
    }

    editorText.setFillColor(sf::Color::Red);
    editorText.setCharacterSize(16);
    editorText.setPosition(10.0f, 10.0f);

    saveMessageText.setFillColor(sf::Color::Green);
    saveMessageText.setCharacterSize(18);
    saveMessageText.setPosition(20.0f, 60.0f);
}

bool EditorController::isFontLoaded() const {
    return editorFont.getInfo().family != "";
}

void EditorController::resetState() {
    selectedPlatformIndex = -1;
    selectedEnemyIndex = -1;
    selectedInteractiveIndex = -1;
    selectedCheckpointIndex = -1;
    selectedPortalIndex = -1;
    isDraggingPlatform = false;
    isDraggingEnemy = false;
    isDraggingInteractive = false;
    isDraggingCheckpoint = false;
    isDraggingPortal = false;
    dragOffset = sf::Vector2f(0.f, 0.f);
    saveMessageTimer = 0.0f;
    saveMessageText.setString("");
}

void EditorController::changeObjectType(ObjectType type) {
    objectType = type;
    selectedPlatformIndex = -1;
    selectedEnemyIndex = -1;
    selectedInteractiveIndex = -1;
    selectedCheckpointIndex = -1;
    selectedPortalIndex = -1;

    if (type == ObjectType::FlameTrap) {
        currentEnemyPreset = EnemyPresetType::FlameHorizontal;
        applyPresetDefaults(currentEnemyPreset);
    } else if (type == ObjectType::RotatingTrap) {
        currentEnemyPreset = EnemyPresetType::RotatingSlow;
        applyPresetDefaults(currentEnemyPreset);
    }
}

sf::Vector2f EditorController::screenToWorld(const sf::Vector2f& screenPos, EditorContext& ctx) const {
    if (!ctx.camera) {
        return screenPos;
    }

    const sf::View& view = ctx.camera->getView();
    return ctx.window.mapPixelToCoords(sf::Vector2i(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y)), view);
}

void EditorController::setSaveMessage(const std::string& message, const sf::Color& color) {
    saveMessageTimer = 2.0f;
    saveMessageText.setString(message);
    saveMessageText.setFillColor(color);
}

void EditorController::reloadLevel(EditorContext& ctx) {
    if (!ctx.currentLevelPath.empty() && ctx.reloadLevel) {
        LevelData* levelData = ctx.reloadLevel(ctx.currentLevelPath);
        ctx.currentLevel = levelData;
        resetState();
    }
}

void EditorController::handleEvent(const sf::Event& event, EditorContext& ctx) {
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(ctx.window);
        sf::Vector2f worldPos = screenToWorld(sf::Vector2f(static_cast<float>(mousePixelPos.x),
                                                           static_cast<float>(mousePixelPos.y)),
                                              ctx);

        if (event.mouseButton.button == sf::Mouse::Left) {
            bool clickedObject = false;

            switch (objectType) {
                case ObjectType::Platform: {
                    for (size_t i = 0; i < ctx.platforms.size(); ++i) {
                        sf::FloatRect bounds = ctx.platforms[i]->getBounds();
                        if (bounds.contains(worldPos)) {
                            selectedPlatformIndex = static_cast<int>(i);
                            selectedEnemyIndex = -1;
                            selectedInteractiveIndex = -1;
                            selectedCheckpointIndex = -1;
                            selectedPortalIndex = -1;
                            isDraggingPlatform = true;
                            isDraggingEnemy = false;
                            isDraggingInteractive = false;
                            isDraggingCheckpoint = false;
                            isDraggingPortal = false;
                            dragOffset = worldPos - sf::Vector2f(bounds.left, bounds.top);
                            clickedObject = true;
                            break;
                        }
                    }
                    if (!clickedObject) {
                        ctx.platforms.push_back(std::make_unique<Platform>(worldPos.x, worldPos.y, 100.0f, 20.0f, Platform::Type::Floor));
                        selectedPlatformIndex = static_cast<int>(ctx.platforms.size() - 1);
                        selectedEnemyIndex = -1;
                        selectedInteractiveIndex = -1;
                        selectedCheckpointIndex = -1;
                        selectedPortalIndex = -1;
                        isDraggingPlatform = true;
                        isDraggingEnemy = false;
                        isDraggingInteractive = false;
                        isDraggingCheckpoint = false;
                        isDraggingPortal = false;
                        dragOffset = sf::Vector2f(0, 0);
                    }
                    break;
                }
                case ObjectType::Terminal:
                case ObjectType::Door:
                case ObjectType::Turret: {
                    for (size_t i = 0; i < ctx.interactiveObjects.size(); ++i) {
                        sf::FloatRect bounds = ctx.interactiveObjects[i]->getBounds();
                        if (bounds.contains(worldPos)) {
                            selectedInteractiveIndex = static_cast<int>(i);
                            selectedPlatformIndex = -1;
                            selectedEnemyIndex = -1;
                            selectedCheckpointIndex = -1;
                            selectedPortalIndex = -1;
                            isDraggingInteractive = true;
                            isDraggingPlatform = false;
                            isDraggingEnemy = false;
                            isDraggingCheckpoint = false;
                            isDraggingPortal = false;
                            dragOffset = worldPos - sf::Vector2f(bounds.left, bounds.top);
                            clickedObject = true;
                            break;
                        }
                    }
                    if (!clickedObject) {
                        InteractiveType type = InteractiveType::Terminal;
                        if (objectType == ObjectType::Door) {
                            type = InteractiveType::Door;
                        } else if (objectType == ObjectType::Turret) {
                            type = InteractiveType::Turret;
                        }
                        std::string id = "interactive_" + std::to_string(ctx.interactiveObjects.size());
                        ctx.interactiveObjects.push_back(std::make_unique<InteractiveObject>(worldPos.x, worldPos.y, 50.0f, 50.0f, type, id));
                        selectedInteractiveIndex = static_cast<int>(ctx.interactiveObjects.size() - 1);
                        selectedPlatformIndex = -1;
                        selectedEnemyIndex = -1;
                        selectedCheckpointIndex = -1;
                        selectedPortalIndex = -1;
                        isDraggingInteractive = true;
                        isDraggingPlatform = false;
                        isDraggingEnemy = false;
                        isDraggingCheckpoint = false;
                        isDraggingPortal = false;
                        dragOffset = sf::Vector2f(0, 0);
                    }
                    break;
                }
                case ObjectType::Checkpoint: {
                    for (size_t i = 0; i < ctx.checkpoints.size(); ++i) {
                        sf::FloatRect bounds = ctx.checkpoints[i]->getBounds();
                        if (bounds.contains(worldPos)) {
                            selectedCheckpointIndex = static_cast<int>(i);
                            selectedPlatformIndex = -1;
                            selectedEnemyIndex = -1;
                            selectedInteractiveIndex = -1;
                            selectedPortalIndex = -1;
                            isDraggingCheckpoint = true;
                            isDraggingPlatform = false;
                            isDraggingEnemy = false;
                            isDraggingInteractive = false;
                            isDraggingPortal = false;
                            dragOffset = worldPos - sf::Vector2f(bounds.left, bounds.top);
                            clickedObject = true;
                            break;
                        }
                    }
                    if (!clickedObject) {
                        std::string id = "cp_" + std::to_string(ctx.checkpoints.size());
                        ctx.checkpoints.push_back(std::make_unique<Checkpoint>(worldPos.x, worldPos.y, id));
                        selectedCheckpointIndex = static_cast<int>(ctx.checkpoints.size() - 1);
                        selectedPlatformIndex = -1;
                        selectedEnemyIndex = -1;
                        selectedInteractiveIndex = -1;
                        selectedPortalIndex = -1;
                        isDraggingCheckpoint = true;
                        isDraggingPlatform = false;
                        isDraggingEnemy = false;
                        isDraggingInteractive = false;
                        isDraggingPortal = false;
                        dragOffset = sf::Vector2f(0, 0);
                    }
                    break;
                }
                case ObjectType::Portal: {
                    if (ctx.currentLevel) {
                        for (size_t i = 0; i < ctx.currentLevel->portals.size(); ++i) {
                            const auto& portal = ctx.currentLevel->portals[i];
                            sf::FloatRect bounds(portal.x, portal.y, portal.width, portal.height);
                            if (bounds.contains(worldPos)) {
                                selectedPortalIndex = static_cast<int>(i);
                                selectedPlatformIndex = -1;
                                selectedEnemyIndex = -1;
                                selectedInteractiveIndex = -1;
                                selectedCheckpointIndex = -1;
                                isDraggingPortal = true;
                                isDraggingPlatform = false;
                                isDraggingEnemy = false;
                                isDraggingInteractive = false;
                                isDraggingCheckpoint = false;
                                dragOffset = worldPos - sf::Vector2f(bounds.left, bounds.top);
                                clickedObject = true;
                                break;
                            }
                        }
                    }
                    if (!clickedObject && ctx.currentLevel) {
                        Portal newPortal{};
                        newPortal.x = worldPos.x;
                        newPortal.y = worldPos.y;
                        newPortal.width = 20.0f;
                        newPortal.height = 200.0f;
                        newPortal.targetLevel = "zone1_level1";
                        newPortal.spawnDirection = "leftbottom";
                        newPortal.useCustomSpawn = false;
                        ctx.currentLevel->portals.push_back(newPortal);
                        selectedPortalIndex = static_cast<int>(ctx.currentLevel->portals.size() - 1);
                        selectedPlatformIndex = -1;
                        selectedEnemyIndex = -1;
                        selectedInteractiveIndex = -1;
                        selectedCheckpointIndex = -1;
                        isDraggingPortal = true;
                        isDraggingPlatform = false;
                        isDraggingEnemy = false;
                        isDraggingInteractive = false;
                        isDraggingCheckpoint = false;
                        dragOffset = sf::Vector2f(0, 0);
                    }
                    break;
                }
                case ObjectType::PatrolEnemy:
                case ObjectType::FlyingEnemy:
                case ObjectType::Spike:
                case ObjectType::FlameTrap:
                case ObjectType::RotatingTrap: {
                    for (size_t i = 0; i < ctx.enemies.size(); ++i) {
                        sf::FloatRect bounds = ctx.enemies[i]->getBounds();
                        if (bounds.contains(worldPos)) {
                            selectedEnemyIndex = static_cast<int>(i);
                            selectedPlatformIndex = -1;
                            selectedInteractiveIndex = -1;
                            selectedCheckpointIndex = -1;
                            selectedPortalIndex = -1;
                            isDraggingEnemy = true;
                            isDraggingPlatform = false;
                            isDraggingInteractive = false;
                            isDraggingCheckpoint = false;
                            isDraggingPortal = false;
                            dragOffset = worldPos - sf::Vector2f(bounds.left, bounds.top);
                            clickedObject = true;
                            break;
                        }
                    }
                    if (!clickedObject) {
                        switch (objectType) {
                            case ObjectType::PatrolEnemy: {
                                EnemyStats stats = getPresetStats(currentEnemyPreset);
                                // Only use ground presets for PatrolEnemy
                                if (currentEnemyPreset == EnemyPresetType::FlyingBasic || 
                                    currentEnemyPreset == EnemyPresetType::FlyingShooter) {
                                    stats = EnemyPresets::Basic();
                                }
                                ctx.enemies.push_back(std::make_unique<PatrolEnemy>(worldPos.x, worldPos.y, 100.0f, stats));
                                break;
                            }
                            case ObjectType::FlyingEnemy: {
                                EnemyStats stats = EnemyPresets::FlyingBasic();
                                // Use flying presets for FlyingEnemy
                                if (currentEnemyPreset == EnemyPresetType::FlyingBasic) {
                                    stats = EnemyPresets::FlyingBasic();
                                } else if (currentEnemyPreset == EnemyPresetType::FlyingShooter) {
                                    stats = EnemyPresets::FlyingShooter();
                                } else {
                                    stats = EnemyPresets::FlyingBasic();
                                }
                                ctx.enemies.push_back(std::make_unique<FlyingEnemy>(worldPos.x, worldPos.y, 200.0f, true, stats));
                                break;
                            }
                            case ObjectType::Spike:
                                ctx.enemies.push_back(std::make_unique<Spike>(worldPos.x, worldPos.y));
                                break;
                            case ObjectType::FlameTrap: {
                                EnemyStats stats = getPresetStats(currentEnemyPreset);
                                auto flame = std::make_unique<FlameTrap>(worldPos.x, worldPos.y, stats);
                                flame->setDirection(currentFlameDirection);
                                flame->setActiveDuration(currentFlameActive);
                                flame->setInactiveDuration(currentFlameInactive);
                                flame->setShotInterval(currentFlameInterval);
                                flame->setProjectileSpeed(currentFlameProjectileSpeed);
                                flame->setProjectileRange(currentFlameProjectileRange);
                                ctx.enemies.push_back(std::move(flame));
                                break;
                            }
                            case ObjectType::RotatingTrap: {
                                EnemyStats stats = getPresetStats(currentEnemyPreset);
                                auto trap = std::make_unique<RotatingTrap>(worldPos.x, worldPos.y, stats);
                                trap->setRotationSpeed(currentRotatingSpeed);
                                trap->setArmLength(currentRotatingArmLength);
                                trap->setArmThickness(currentRotatingArmThickness);
                                ctx.enemies.push_back(std::move(trap));
                                break;
                            }
                            default:
                                break;
                        }
                        selectedEnemyIndex = static_cast<int>(ctx.enemies.size() - 1);
                        selectedPlatformIndex = -1;
                        selectedInteractiveIndex = -1;
                        selectedCheckpointIndex = -1;
                        selectedPortalIndex = -1;
                        isDraggingEnemy = true;
                        isDraggingPlatform = false;
                        isDraggingInteractive = false;
                        isDraggingCheckpoint = false;
                        isDraggingPortal = false;
                        dragOffset = sf::Vector2f(0, 0);
                    }
                    break;
                }
            }
        } else if (event.mouseButton.button == sf::Mouse::Right) {
            bool deleted = false;
            for (size_t i = 0; i < ctx.platforms.size(); ++i) {
                sf::FloatRect bounds = ctx.platforms[i]->getBounds();
                if (bounds.contains(worldPos)) {
                    ctx.platforms.erase(ctx.platforms.begin() + i);
                    if (selectedPlatformIndex == static_cast<int>(i)) {
                        selectedPlatformIndex = -1;
                    } else if (selectedPlatformIndex > static_cast<int>(i)) {
                        selectedPlatformIndex--;
                    }
                    deleted = true;
                    break;
                }
            }
            if (!deleted) {
                for (size_t i = 0; i < ctx.interactiveObjects.size(); ++i) {
                    sf::FloatRect bounds = ctx.interactiveObjects[i]->getBounds();
                    if (bounds.contains(worldPos)) {
                        ctx.interactiveObjects.erase(ctx.interactiveObjects.begin() + i);
                        if (selectedInteractiveIndex == static_cast<int>(i)) {
                            selectedInteractiveIndex = -1;
                        } else if (selectedInteractiveIndex > static_cast<int>(i)) {
                            selectedInteractiveIndex--;
                        }
                        deleted = true;
                        break;
                    }
                }
            }
            if (!deleted) {
                for (size_t i = 0; i < ctx.checkpoints.size(); ++i) {
                    sf::FloatRect bounds = ctx.checkpoints[i]->getBounds();
                    if (bounds.contains(worldPos)) {
                        ctx.checkpoints.erase(ctx.checkpoints.begin() + i);
                        if (selectedCheckpointIndex == static_cast<int>(i)) {
                            selectedCheckpointIndex = -1;
                        } else if (selectedCheckpointIndex > static_cast<int>(i)) {
                            selectedCheckpointIndex--;
                        }
                        deleted = true;
                        break;
                    }
                }
            }
            if (!deleted) {
                for (size_t i = 0; i < ctx.enemies.size(); ++i) {
                    sf::FloatRect bounds = ctx.enemies[i]->getBounds();
                    if (bounds.contains(worldPos)) {
                        ctx.enemies.erase(ctx.enemies.begin() + i);
                        if (selectedEnemyIndex == static_cast<int>(i)) {
                            selectedEnemyIndex = -1;
                        } else if (selectedEnemyIndex > static_cast<int>(i)) {
                            selectedEnemyIndex--;
                        }
                        deleted = true;
                        break;
                    }
                }
            }
            if (!deleted && ctx.currentLevel) {
                for (size_t i = 0; i < ctx.currentLevel->portals.size(); ++i) {
                    Portal& portal = ctx.currentLevel->portals[i];
                    sf::FloatRect bounds(portal.x, portal.y, portal.width, portal.height);
                    if (bounds.contains(worldPos)) {
                        ctx.currentLevel->portals.erase(ctx.currentLevel->portals.begin() + i);
                        if (selectedPortalIndex == static_cast<int>(i)) {
                            selectedPortalIndex = -1;
                        } else if (selectedPortalIndex > static_cast<int>(i)) {
                            selectedPortalIndex--;
                        }
                        break;
                    }
                }
            }
        }
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        isDraggingPlatform = false;
        isDraggingEnemy = false;
        isDraggingInteractive = false;
        isDraggingCheckpoint = false;
        isDraggingPortal = false;
    }

    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Num1: changeObjectType(ObjectType::Platform); break;
            case sf::Keyboard::Num2: changeObjectType(ObjectType::PatrolEnemy); break;
            case sf::Keyboard::Num3: changeObjectType(ObjectType::FlyingEnemy); break;
            case sf::Keyboard::Num0: changeObjectType(ObjectType::FlameTrap); break;
            case sf::Keyboard::Dash: changeObjectType(ObjectType::RotatingTrap); break;
            case sf::Keyboard::F: changeObjectType(ObjectType::FlameTrap); break;
            case sf::Keyboard::R: changeObjectType(ObjectType::RotatingTrap); break;
            
            // Change enemy preset with P key
            case sf::Keyboard::P: {
                if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
                    // Cycle through presets for selected enemy
                    int presetCount = static_cast<int>(EnemyPresetType::RotatingFast) + 1;
                    int currentPresetInt = static_cast<int>(currentEnemyPreset);
                    currentPresetInt = (currentPresetInt + 1) % presetCount;
                    currentEnemyPreset = static_cast<EnemyPresetType>(currentPresetInt);
                    
                    // Apply preset to selected enemy
                    Enemy* enemy = ctx.enemies[selectedEnemyIndex].get();
                    if (enemy) {
                        applyPresetToEnemy(enemy, currentEnemyPreset, ctx);
                    }
                } else {
                    // Cycle preset for future enemies
                    int presetCount = static_cast<int>(EnemyPresetType::RotatingFast) + 1;
                    int currentPresetInt = static_cast<int>(currentEnemyPreset);
                    currentPresetInt = (currentPresetInt + 1) % presetCount;
                    currentEnemyPreset = static_cast<EnemyPresetType>(currentPresetInt);
                    applyPresetDefaults(currentEnemyPreset);
                }
                break;
            }
            case sf::Keyboard::Num4: changeObjectType(ObjectType::Spike); break;
            case sf::Keyboard::Num5: changeObjectType(ObjectType::Terminal); break;
            case sf::Keyboard::Num6: changeObjectType(ObjectType::Door); break;
            case sf::Keyboard::Num7: changeObjectType(ObjectType::Turret); break;
            case sf::Keyboard::Num8: changeObjectType(ObjectType::Checkpoint); break;
            case sf::Keyboard::Num9: changeObjectType(ObjectType::Portal); break;
            default: break;
        }

        if (event.key.code == sf::Keyboard::Y) {
            auto advanceDirection = [](FlameDirection dir) {
                switch (dir) {
                    case FlameDirection::Left: return FlameDirection::Right;
                    case FlameDirection::Right: return FlameDirection::Up;
                    case FlameDirection::Up: return FlameDirection::Down;
                    case FlameDirection::Down: return FlameDirection::Left;
                }
                return FlameDirection::Right;
            };

            if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
                if (auto* flame = dynamic_cast<FlameTrap*>(ctx.enemies[selectedEnemyIndex].get())) {
                    flame->cycleDirection();
                    currentFlameDirection = flame->getDirection();
                }
            } else {
                currentFlameDirection = advanceDirection(currentFlameDirection);
            }
        } else if (event.key.code == sf::Keyboard::U) {
            if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
                if (auto* rotating = dynamic_cast<RotatingTrap*>(ctx.enemies[selectedEnemyIndex].get())) {
                    rotating->toggleDirection();
                    currentRotatingSpeed = rotating->getRotationSpeed();
                }
            } else {
                currentRotatingSpeed = -currentRotatingSpeed;
            }
        } else if (event.key.code == sf::Keyboard::I) {
            auto adjustSpeed = [](float speed, float delta) {
                float magnitude = std::max(20.0f, std::abs(speed) + delta);
                float sign = (speed >= 0.0f) ? 1.0f : -1.0f;
                return sign * magnitude;
            };
            if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
                if (auto* rotating = dynamic_cast<RotatingTrap*>(ctx.enemies[selectedEnemyIndex].get())) {
                    float newSpeed = adjustSpeed(rotating->getRotationSpeed(), 20.0f);
                    rotating->setRotationSpeed(newSpeed);
                    currentRotatingSpeed = newSpeed;
                }
            } else {
                currentRotatingSpeed = adjustSpeed(currentRotatingSpeed, 20.0f);
            }
        } else if (event.key.code == sf::Keyboard::K) {
            auto adjustSpeed = [](float speed, float delta) {
                float sign = (speed >= 0.0f) ? 1.0f : -1.0f;
                float magnitude = std::max(20.0f, std::abs(speed) - delta);
                return sign * magnitude;
            };
            if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
                if (auto* rotating = dynamic_cast<RotatingTrap*>(ctx.enemies[selectedEnemyIndex].get())) {
                    float newSpeed = adjustSpeed(rotating->getRotationSpeed(), 20.0f);
                    rotating->setRotationSpeed(newSpeed);
                    currentRotatingSpeed = newSpeed;
                }
            } else {
                currentRotatingSpeed = adjustSpeed(currentRotatingSpeed, 20.0f);
            }
        } else if (event.key.code == sf::Keyboard::L) {
            if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
                if (auto* rotating = dynamic_cast<RotatingTrap*>(ctx.enemies[selectedEnemyIndex].get())) {
                    float newLength = rotating->getArmLength() + 10.0f;
                    rotating->setArmLength(newLength);
                    currentRotatingArmLength = newLength;
                }
            } else {
                currentRotatingArmLength += 10.0f;
            }
        } else if (event.key.code == sf::Keyboard::J) {
            if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
                if (auto* rotating = dynamic_cast<RotatingTrap*>(ctx.enemies[selectedEnemyIndex].get())) {
                    float newLength = std::max(40.0f, rotating->getArmLength() - 10.0f);
                    rotating->setArmLength(newLength);
                    currentRotatingArmLength = newLength;
                }
            } else {
                currentRotatingArmLength = std::max(40.0f, currentRotatingArmLength - 10.0f);
            }
        }
    }

    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::S &&
        sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
        saveLevel(ctx);
    }

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Delete) {
        if (selectedPlatformIndex >= 0 && selectedPlatformIndex < static_cast<int>(ctx.platforms.size())) {
            ctx.platforms.erase(ctx.platforms.begin() + selectedPlatformIndex);
            selectedPlatformIndex = -1;
        } else if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
            ctx.enemies.erase(ctx.enemies.begin() + selectedEnemyIndex);
            selectedEnemyIndex = -1;
        } else if (selectedInteractiveIndex >= 0 && selectedInteractiveIndex < static_cast<int>(ctx.interactiveObjects.size())) {
            ctx.interactiveObjects.erase(ctx.interactiveObjects.begin() + selectedInteractiveIndex);
            selectedInteractiveIndex = -1;
        } else if (selectedCheckpointIndex >= 0 && selectedCheckpointIndex < static_cast<int>(ctx.checkpoints.size())) {
            ctx.checkpoints.erase(ctx.checkpoints.begin() + selectedCheckpointIndex);
            selectedCheckpointIndex = -1;
        } else if (selectedPortalIndex >= 0 && ctx.currentLevel && selectedPortalIndex < static_cast<int>(ctx.currentLevel->portals.size())) {
            ctx.currentLevel->portals.erase(ctx.currentLevel->portals.begin() + selectedPortalIndex);
            selectedPortalIndex = -1;
        }
    }

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F5) {
        reloadLevel(ctx);
    }

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::T) {
        if (selectedPortalIndex >= 0 && ctx.currentLevel && selectedPortalIndex < static_cast<int>(ctx.currentLevel->portals.size())) {
            Portal& portal = ctx.currentLevel->portals[selectedPortalIndex];
            std::string currentDir = portal.spawnDirection;
            while (!currentDir.empty() && (currentDir.front() == '"' || currentDir.front() == '\'')) {
                currentDir = currentDir.substr(1);
            }
            while (!currentDir.empty() && (currentDir.back() == '"' || currentDir.back() == '\'')) {
                currentDir = currentDir.substr(0, currentDir.length() - 1);
            }
            std::string lowerDir = currentDir;
            std::transform(lowerDir.begin(), lowerDir.end(), lowerDir.begin(), ::tolower);
            if (lowerDir == "lefttop") {
                portal.spawnDirection = "leftbottom";
            } else if (lowerDir == "leftbottom") {
                portal.spawnDirection = "righttop";
            } else if (lowerDir == "righttop") {
                portal.spawnDirection = "rightbottom";
            } else if (lowerDir == "rightbottom") {
                portal.spawnDirection = "lefttop";
            } else {
                portal.spawnDirection = "lefttop";
            }
            std::cout << "Spawn direction changee: " << portal.spawnDirection << "\n";
        } else if (selectedPlatformIndex >= 0 && selectedPlatformIndex < static_cast<int>(ctx.platforms.size())) {
            Platform::Type currentType = ctx.platforms[selectedPlatformIndex]->getType();
            Platform::Type nextType = Platform::Type::Floor;
            switch (currentType) {
                case Platform::Type::Floor: nextType = Platform::Type::EndFloor; break;
                case Platform::Type::EndFloor: nextType = Platform::Type::Floor; break;
                default: break;
            }
            ctx.platforms[selectedPlatformIndex]->setType(nextType);
            std::cout << "Type de plateforme change\n";
        }
    }

    if (event.type == sf::Event::KeyPressed && selectedPlatformIndex >= 0 && selectedPlatformIndex < static_cast<int>(ctx.platforms.size())) {
        sf::Vector2f currentSize = ctx.platforms[selectedPlatformIndex]->getSize();
        float resizeStep = 10.0f;
        if (event.key.code == sf::Keyboard::Add || event.key.code == sf::Keyboard::Equal) {
            ctx.platforms[selectedPlatformIndex]->setSize(currentSize.x + resizeStep, currentSize.y);
        } else if (event.key.code == sf::Keyboard::Subtract || event.key.code == sf::Keyboard::Hyphen) {
            ctx.platforms[selectedPlatformIndex]->setSize(std::max(10.0f, currentSize.x - resizeStep), currentSize.y);
        } else if (event.key.code == sf::Keyboard::PageUp) {
            ctx.platforms[selectedPlatformIndex]->setSize(currentSize.x, currentSize.y + resizeStep);
        } else if (event.key.code == sf::Keyboard::PageDown) {
            ctx.platforms[selectedPlatformIndex]->setSize(currentSize.x, std::max(10.0f, currentSize.y - resizeStep));
        }
    }

    if (event.type == sf::Event::KeyPressed && selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
        Enemy* enemy = ctx.enemies[selectedEnemyIndex].get();
        float currentDistance = enemy->getPatrolDistance();
        float distanceStep = 20.0f;

        if (event.key.code == sf::Keyboard::Q) {
            enemy->setPatrolDistance(std::max(20.0f, currentDistance - distanceStep));
        } else if (event.key.code == sf::Keyboard::W) {
            enemy->setPatrolDistance(currentDistance + distanceStep);
        }
    }
}

void EditorController::update(float dt, EditorContext& ctx) {
    if (saveMessageTimer > 0.0f) {
        saveMessageTimer -= dt;
    }

    if (!ctx.camera) {
        return;
    }

    Player* player = ctx.activePlayer;
    if (player) {
        ctx.camera->update(player->getPosition(), dt);
        ctx.camera->apply(ctx.window);
    }

    if (isDraggingPlatform && selectedPlatformIndex >= 0 && selectedPlatformIndex < static_cast<int>(ctx.platforms.size())) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(ctx.window);
        sf::Vector2f worldPos = screenToWorld(sf::Vector2f(static_cast<float>(mousePixelPos.x), static_cast<float>(mousePixelPos.y)), ctx);
        ctx.platforms[selectedPlatformIndex]->setPosition(worldPos.x - dragOffset.x, worldPos.y - dragOffset.y);
    }

    if (isDraggingEnemy && selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(ctx.window);
        sf::Vector2f worldPos = screenToWorld(sf::Vector2f(static_cast<float>(mousePixelPos.x), static_cast<float>(mousePixelPos.y)), ctx);
        Enemy* enemy = ctx.enemies[selectedEnemyIndex].get();
        sf::Vector2f newPos(worldPos.x - dragOffset.x, worldPos.y - dragOffset.y);
        enemy->setPosition(newPos.x, newPos.y);
        float currentDistance = enemy->getPatrolDistance();
        enemy->setPatrolBounds(newPos.x - currentDistance / 2.0f, newPos.x + currentDistance / 2.0f);
        if (auto* flyingEnemy = dynamic_cast<FlyingEnemy*>(enemy)) {
            float topBound = flyingEnemy->getTopBound();
            float bottomBound = flyingEnemy->getBottomBound();
            if (topBound != 0.0f || bottomBound != 0.0f) {
                float verticalDistance = bottomBound - topBound;
                flyingEnemy->setVerticalPatrolBounds(newPos.y - verticalDistance / 2.0f, newPos.y + verticalDistance / 2.0f);
            }
        }
    }

    if (isDraggingInteractive && selectedInteractiveIndex >= 0 && selectedInteractiveIndex < static_cast<int>(ctx.interactiveObjects.size())) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(ctx.window);
        sf::Vector2f worldPos = screenToWorld(sf::Vector2f(static_cast<float>(mousePixelPos.x), static_cast<float>(mousePixelPos.y)), ctx);
        ctx.interactiveObjects[selectedInteractiveIndex]->setPosition(worldPos.x - dragOffset.x, worldPos.y - dragOffset.y);
    }

    if (isDraggingCheckpoint && selectedCheckpointIndex >= 0 && selectedCheckpointIndex < static_cast<int>(ctx.checkpoints.size())) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(ctx.window);
        sf::Vector2f worldPos = screenToWorld(sf::Vector2f(static_cast<float>(mousePixelPos.x), static_cast<float>(mousePixelPos.y)), ctx);
        ctx.checkpoints[selectedCheckpointIndex]->setPosition(worldPos.x - dragOffset.x, worldPos.y - dragOffset.y);
    }

    if (isDraggingPortal && ctx.currentLevel && selectedPortalIndex >= 0 && selectedPortalIndex < static_cast<int>(ctx.currentLevel->portals.size())) {
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(ctx.window);
        sf::Vector2f worldPos = screenToWorld(sf::Vector2f(static_cast<float>(mousePixelPos.x), static_cast<float>(mousePixelPos.y)), ctx);
        Portal& portal = ctx.currentLevel->portals[selectedPortalIndex];
        portal.x = worldPos.x - dragOffset.x;
        portal.y = worldPos.y - dragOffset.y;
    }

    if (player) {
        const float moveSpeed = 500.0f * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            player->setPosition(player->getPosition().x - moveSpeed, player->getPosition().y);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            player->setPosition(player->getPosition().x + moveSpeed, player->getPosition().y);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            player->setPosition(player->getPosition().x, player->getPosition().y - moveSpeed);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            player->setPosition(player->getPosition().x, player->getPosition().y + moveSpeed);
        }
    }
}

void EditorController::render(EditorContext& ctx) {
    if (!ctx.camera) {
        return;
    }

    if (ctx.activePlayer) {
        ctx.camera->apply(ctx.window);
    }

    for (size_t i = 0; i < ctx.platforms.size(); ++i) {
        ctx.platforms[i]->draw(ctx.window);
        if (static_cast<int>(i) == selectedPlatformIndex) {
            sf::FloatRect bounds = ctx.platforms[i]->getBounds();
            sf::RectangleShape outline;
            outline.setSize(sf::Vector2f(bounds.width, bounds.height));
            outline.setPosition(bounds.left, bounds.top);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::Yellow);
            outline.setOutlineThickness(2.0f);
            ctx.window.draw(outline);
        }
        if (isFontLoaded()) {
            sf::Text indexText;
            indexText.setFont(editorFont);
            indexText.setString(std::to_string(i));
            indexText.setCharacterSize(12);
            indexText.setFillColor(sf::Color::White);
            sf::FloatRect bounds = ctx.platforms[i]->getBounds();
            indexText.setPosition(bounds.left + bounds.width / 2.0f - 10.0f, bounds.top - 15.0f);
            ctx.window.draw(indexText);
        }
    }

    for (size_t i = 0; i < ctx.enemies.size(); ++i) {
        Enemy* enemy = ctx.enemies[i].get();
        if (!enemy) continue; // Skip null enemies
        
        // Draw enemy even if dead (forceDraw = true for editor)
        enemy->draw(ctx.window, true);

        if (enemy->getType() == EnemyType::Patrol || enemy->getType() == EnemyType::Flying) {
            sf::Vector2f pos = enemy->getPosition();
            FlyingEnemy* flyingEnemy = dynamic_cast<FlyingEnemy*>(enemy);
            bool isVerticalPatrol = false;
            float topBound = 0.0f, bottomBound = 0.0f;
            if (flyingEnemy) {
                topBound = flyingEnemy->getTopBound();
                bottomBound = flyingEnemy->getBottomBound();
                isVerticalPatrol = (topBound != 0.0f || bottomBound != 0.0f);
            }
            if (isVerticalPatrol) {
                sf::RectangleShape patrolLine;
                patrolLine.setSize(sf::Vector2f(2.0f, bottomBound - topBound));
                patrolLine.setPosition(pos.x - 40.0f, topBound);
                patrolLine.setFillColor(sf::Color(255, 255, 0, 100));
                ctx.window.draw(patrolLine);

                sf::CircleShape topMarker(3.0f);
                topMarker.setPosition(pos.x - 41.0f, topBound - 3.0f);
                topMarker.setFillColor(sf::Color::Yellow);
                ctx.window.draw(topMarker);

                sf::CircleShape bottomMarker(3.0f);
                bottomMarker.setPosition(pos.x - 41.0f, bottomBound - 3.0f);
                bottomMarker.setFillColor(sf::Color::Yellow);
                ctx.window.draw(bottomMarker);
            } else {
                float leftBound = enemy->getLeftBound();
                float rightBound = enemy->getRightBound();
                sf::RectangleShape patrolLine;
                patrolLine.setSize(sf::Vector2f(rightBound - leftBound, 2.0f));
                patrolLine.setPosition(leftBound, pos.y - 40.0f);
                patrolLine.setFillColor(sf::Color(255, 255, 0, 100));
                ctx.window.draw(patrolLine);

                sf::CircleShape leftMarker(3.0f);
                leftMarker.setPosition(leftBound - 3.0f, pos.y - 41.0f);
                leftMarker.setFillColor(sf::Color::Yellow);
                ctx.window.draw(leftMarker);

                sf::CircleShape rightMarker(3.0f);
                rightMarker.setPosition(rightBound - 3.0f, pos.y - 41.0f);
                rightMarker.setFillColor(sf::Color::Yellow);
                ctx.window.draw(rightMarker);
            }
        }

        if (static_cast<int>(i) == selectedEnemyIndex) {
            sf::FloatRect bounds = ctx.enemies[i]->getBounds();
            sf::RectangleShape outline;
            outline.setSize(sf::Vector2f(bounds.width, bounds.height));
            outline.setPosition(bounds.left, bounds.top);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::Cyan);
            outline.setOutlineThickness(2.0f);
            ctx.window.draw(outline);
        }

        if (isFontLoaded()) {
            sf::Text indexText;
            indexText.setFont(editorFont);
            std::string info = "E" + std::to_string(i);
            if (enemy->getType() == EnemyType::Patrol || enemy->getType() == EnemyType::Flying) {
                info += " (" + std::to_string(static_cast<int>(enemy->getPatrolDistance())) + ")";
            }
            indexText.setString(info);
            indexText.setCharacterSize(12);
            indexText.setFillColor(sf::Color::Cyan);
            sf::FloatRect bounds = ctx.enemies[i]->getBounds();
            indexText.setPosition(bounds.left + bounds.width / 2.0f - 20.0f, bounds.top - 15.0f);
            ctx.window.draw(indexText);
        }
    }

    for (size_t i = 0; i < ctx.interactiveObjects.size(); ++i) {
        ctx.interactiveObjects[i]->draw(ctx.window);
        if (static_cast<int>(i) == selectedInteractiveIndex) {
            sf::FloatRect bounds = ctx.interactiveObjects[i]->getBounds();
            sf::RectangleShape outline;
            outline.setSize(sf::Vector2f(bounds.width, bounds.height));
            outline.setPosition(bounds.left, bounds.top);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::Magenta);
            outline.setOutlineThickness(2.0f);
            ctx.window.draw(outline);
        }
        if (isFontLoaded()) {
            sf::Text indexText;
            indexText.setFont(editorFont);
            std::string typeStr = "T";
            switch (ctx.interactiveObjects[i]->getType()) {
                case InteractiveType::Terminal: typeStr = "Term"; break;
                case InteractiveType::Door: typeStr = "Door"; break;
                case InteractiveType::Turret: typeStr = "Turr"; break;
            }
            indexText.setString("I" + std::to_string(i) + " " + typeStr);
            indexText.setCharacterSize(12);
            indexText.setFillColor(sf::Color::Magenta);
            sf::FloatRect bounds = ctx.interactiveObjects[i]->getBounds();
            indexText.setPosition(bounds.left + bounds.width / 2.0f - 25.0f, bounds.top - 15.0f);
            ctx.window.draw(indexText);
        }
    }

    for (size_t i = 0; i < ctx.checkpoints.size(); ++i) {
        ctx.checkpoints[i]->draw(ctx.window);
        if (static_cast<int>(i) == selectedCheckpointIndex) {
            sf::FloatRect bounds = ctx.checkpoints[i]->getBounds();
            sf::RectangleShape outline;
            outline.setSize(sf::Vector2f(bounds.width, bounds.height));
            outline.setPosition(bounds.left, bounds.top);
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineColor(sf::Color::Green);
            outline.setOutlineThickness(2.0f);
            ctx.window.draw(outline);
        }
        if (isFontLoaded()) {
            sf::Text indexText;
            indexText.setFont(editorFont);
            indexText.setString("CP" + std::to_string(i) + "\n" + ctx.checkpoints[i]->getId());
            indexText.setCharacterSize(12);
            indexText.setFillColor(sf::Color::Green);
            sf::FloatRect bounds = ctx.checkpoints[i]->getBounds();
            indexText.setPosition(bounds.left + bounds.width / 2.0f - 30.0f, bounds.top - 30.0f);
            ctx.window.draw(indexText);
        }
    }

    if (ctx.currentLevel) {
        for (size_t i = 0; i < ctx.currentLevel->portals.size(); ++i) {
            const auto& portal = ctx.currentLevel->portals[i];
            sf::RectangleShape portalRect;
            portalRect.setSize(sf::Vector2f(portal.width, portal.height));
            portalRect.setPosition(portal.x, portal.y);
            portalRect.setFillColor(sf::Color(255, 0, 255, 100));
            portalRect.setOutlineColor(sf::Color::Magenta);
            portalRect.setOutlineThickness(2.0f);
            ctx.window.draw(portalRect);

            if (static_cast<int>(i) == selectedPortalIndex) {
                sf::RectangleShape outline;
                outline.setSize(sf::Vector2f(portal.width + 4.0f, portal.height + 4.0f));
                outline.setPosition(portal.x - 2.0f, portal.y - 2.0f);
                outline.setFillColor(sf::Color::Transparent);
                outline.setOutlineColor(sf::Color::Yellow);
                outline.setOutlineThickness(3.0f);
                ctx.window.draw(outline);
            }

            if (isFontLoaded()) {
                sf::Text indexText;
                indexText.setFont(editorFont);
                std::string info = "Portal" + std::to_string(i) + "\n" + portal.targetLevel + "\n" + portal.spawnDirection;
                indexText.setString(info);
                indexText.setCharacterSize(10);
                indexText.setFillColor(sf::Color::Magenta);
                indexText.setPosition(portal.x + portal.width / 2.0f - 40.0f, portal.y - 45.0f);
                ctx.window.draw(indexText);
            }
        }
    }

    ctx.window.setView(ctx.window.getDefaultView());

    if (isFontLoaded()) {
        editorText.setString(
            "MODE EDITEUR\n"
            "F1: Toggle Editor\n"
            "1-9: Type objet (" +
            [&]() -> std::string {
                switch (objectType) {
                    case ObjectType::Platform: return "Platform";
                    case ObjectType::PatrolEnemy: return "PatrolEnemy";
                    case ObjectType::FlyingEnemy: return "FlyingEnemy";
                    case ObjectType::Spike: return "Spike";
                    case ObjectType::FlameTrap: return "FlameTrap";
                    case ObjectType::RotatingTrap: return "RotatingTrap";
                    case ObjectType::Terminal: return "Terminal";
                    case ObjectType::Door: return "Door";
                    case ObjectType::Turret: return "Turret";
                    case ObjectType::Checkpoint: return "Checkpoint";
                    case ObjectType::Portal: return "Portal";
                }
                return "Platform";
            }() +
            ")\n"
            "  1=Platform 2=Patrol 3=Flying 4=Spike 0=Flame - =Rotating\n"
            "  5=Terminal 6=Door 7=Turret 8=Checkpoint 9=Portal (F=Flame R=Rotating)\n"
            "Clic Gauche: Placer/Selectionner\n"
            "Clic Droit: Supprimer\n"
            "Delete: Supprimer selectionnee\n"
            "Ctrl+S: Sauvegarder\n"
            "F5: Recharger depuis fichier\n"
            "Fleches: Deplacer camera\n"
            "+/-: Largeur plateforme\n"
            "PageUp/Down: Hauteur plateforme\n"
            "T: Changer type plateforme / direction spawn portail\n"
            "Y: Direction flamme  U: Inverser rotation\n"
            "I/K: Ajuster vitesse rotation  J/L: Longueur bras\n"
            "P: Changer preset ennemi (" + getPresetName(currentEnemyPreset) + ")\n"
            "Plateformes: " + std::to_string(ctx.platforms.size()) + "\n"
            "Ennemis: " + std::to_string(ctx.enemies.size()) + "\n"
            "Objets interactifs: " + std::to_string(ctx.interactiveObjects.size()));

        if (selectedEnemyIndex >= 0 && selectedEnemyIndex < static_cast<int>(ctx.enemies.size())) {
            Enemy* enemy = ctx.enemies[selectedEnemyIndex].get();
            if (enemy->getType() == EnemyType::Patrol || enemy->getType() == EnemyType::Flying) {
                editorText.setString(editorText.getString() + "\nQ/W: Distance patrouille (" + std::to_string(static_cast<int>(enemy->getPatrolDistance())) + ")");
                editorText.setString(editorText.getString() + "\nP: Changer preset (actuel: " + getPresetName(currentEnemyPreset) + ")");
                editorText.setString(editorText.getString() + "\nHP: " + std::to_string(enemy->getHP()) + "/" + std::to_string(enemy->getMaxHP()));
                editorText.setString(editorText.getString() + "\nVitesse: " + std::to_string(static_cast<int>(enemy->getStats().speed)));
                editorText.setString(editorText.getString() + "\nDégâts: " + std::to_string(enemy->getStats().damage));
                if (enemy->getStats().canShoot) {
                    editorText.setString(editorText.getString() + "\nTire: Oui (cooldown: " + std::to_string(static_cast<int>(enemy->getStats().shootCooldown)) + "s)");
                } else {
                    editorText.setString(editorText.getString() + "\nTire: Non");
                }
            } else if (enemy->getType() == EnemyType::FlameTrap) {
                if (auto* flame = dynamic_cast<FlameTrap*>(enemy)) {
                    auto directionToString = [](FlameDirection dir) {
                        switch (dir) {
                            case FlameDirection::Left:  return "Left";
                            case FlameDirection::Right: return "Right";
                            case FlameDirection::Up:    return "Up";
                            case FlameDirection::Down:  return "Down";
                        }
                        return "Right";
                    };
                    editorText.setString(editorText.getString() + "\nFlameTrap\nP: Changer preset (" + getPresetName(currentEnemyPreset) + ")");
                    editorText.setString(editorText.getString() + "\nDirection: " + std::string(directionToString(flame->getDirection())));
                    editorText.setString(editorText.getString() + "\nActive: " + std::to_string(static_cast<int>(flame->getActiveDuration() * 1000)) + " ms");
                    editorText.setString(editorText.getString() + "\nRepos: " + std::to_string(static_cast<int>(flame->getInactiveDuration() * 1000)) + " ms");
                    editorText.setString(editorText.getString() + "\nInterval tir: " + std::to_string(static_cast<int>(flame->getShotInterval() * 1000)) + " ms");
                    editorText.setString(editorText.getString() + "\nProjectiles: " + std::to_string(static_cast<int>(flame->getProjectileSpeed())) + " px/s sur " + std::to_string(static_cast<int>(flame->getProjectileRange())) + " px");
                }
            } else if (enemy->getType() == EnemyType::RotatingTrap) {
                if (auto* rotating = dynamic_cast<RotatingTrap*>(enemy)) {
                    editorText.setString(editorText.getString() + "\nRotatingTrap\nP: Changer preset (" + getPresetName(currentEnemyPreset) + ")");
                    editorText.setString(editorText.getString() + "\nVitesse: " + std::to_string(static_cast<int>(rotating->getRotationSpeed())) + " deg/s");
                    editorText.setString(editorText.getString() + "\nLongueur bras: " + std::to_string(static_cast<int>(rotating->getArmLength())) + " px");
                    editorText.setString(editorText.getString() + "\nEpaisseur: " + std::to_string(static_cast<int>(rotating->getArmThickness())) + " px");
                }
            } else if (enemy->getType() == EnemyType::Stationary) {
                editorText.setString(editorText.getString() + "\nSpike (statique)\nP: Preset non disponible");
            }
        }

        ctx.window.draw(editorText);
    }

    if (saveMessageTimer > 0.0f && isFontLoaded()) {
        float alpha = std::min(255.0f, saveMessageTimer * 255.0f / 2.0f);
        sf::Color color = saveMessageText.getFillColor();
        color.a = static_cast<sf::Uint8>(alpha);
        saveMessageText.setFillColor(color);
        ctx.window.draw(saveMessageText);
    }
}

void EditorController::saveLevel(EditorContext& ctx) {
    if (ctx.currentLevelPath.empty()) {
        std::cout << "Erreur: Pas de niveau charge pour sauvegarder\n";
        setSaveMessage("Erreur: Pas de niveau", sf::Color::Red);
        return;
    }

    auto cleanString = [](std::string value) {
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
            value.pop_back();
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
            value.pop_back();
        }
        while (!value.empty() && (value.front() == '"' || value.front() == '\'')) {
            value.erase(value.begin());
        }
        while (!value.empty() && (value.back() == '"' || value.back() == '\'')) {
            value.pop_back();
        }
        return value;
    };

    std::string filename;
    size_t lastSlash = ctx.currentLevelPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = ctx.currentLevelPath.substr(lastSlash + 1);
    } else {
        filename = ctx.currentLevelPath;
    }

    std::string savePath;
    try {
        std::filesystem::path currentDir = std::filesystem::current_path();
        std::filesystem::path sourcePath = currentDir;
        bool foundRoot = false;
        for (int i = 0; i < 10; ++i) {
            if (sourcePath.filename() == "nouveauprojet" || sourcePath.filename() == "PlatformerGame") {
                foundRoot = true;
                break;
            }
            if (sourcePath.has_parent_path()) {
                sourcePath = sourcePath.parent_path();
            } else {
                break;
            }
        }
        if (foundRoot) {
            if (sourcePath.filename() == "nouveauprojet") {
                savePath = (sourcePath / "PlatformerGame" / "assets" / "levels" / filename).string();
            } else {
                savePath = (sourcePath / "assets" / "levels" / filename).string();
            }
            std::cout << "Chemin source construit: " << savePath << "\n";
        } else {
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
        savePath = "../../../PlatformerGame/assets/levels/" + filename;
        std::cout << "Erreur filesystem, utilisation chemin par defaut: " << savePath << "\n";
    }

#if GAME_HAS_JSON
    std::ifstream file(savePath);
    if (!file.is_open()) {
        std::cout << "Erreur: Impossible d'ouvrir " << savePath << " pour sauvegarde\n";
        setSaveMessage("Erreur: Fichier introuvable", sf::Color::Red);
        return;
    }

    nlohmann::json j;
    try {
        file >> j;
        file.close();
    } catch (const std::exception& e) {
        std::cout << "Erreur lors du parsing JSON: " << e.what() << "\n";
        std::cout << "Utilisation du fallback manuel\n";
        file.close();
        goto fallback_save;
    }

    j["platforms"] = nlohmann::json::array();
    for (const auto& platform : ctx.platforms) {
        sf::FloatRect bounds = platform->getBounds();
        nlohmann::json p;
        p["x"] = bounds.left;
        p["y"] = bounds.top;
        p["width"] = bounds.width;
        p["height"] = bounds.height;
        std::string typeStr = (platform->getType() == Platform::Type::EndFloor) ? "endfloor" : "floor";
        p["type"] = typeStr;
        j["platforms"].push_back(p);
    }

    j["enemies"] = nlohmann::json::array();
    auto flameDirToString = [](FlameDirection dir) {
        switch (dir) {
            case FlameDirection::Left:  return std::string("left");
            case FlameDirection::Right: return std::string("right");
            case FlameDirection::Up:    return std::string("up");
            case FlameDirection::Down:  return std::string("down");
        }
        return std::string("right");
    };
    for (const auto& enemy : ctx.enemies) {
        if (!enemy) continue; // Skip null enemies
        sf::Vector2f pos = enemy->getPosition();
        nlohmann::json e;
        e["x"] = pos.x;
        e["y"] = pos.y;
        if (auto* patrol = dynamic_cast<PatrolEnemy*>(enemy.get())) {
            e["type"] = "patrol";
            e["patrolDistance"] = patrol->getPatrolDistance();
            const EnemyStats& stats = enemy->getStats();
            e["maxHP"] = stats.maxHP;
            e["sizeX"] = stats.sizeX;
            e["sizeY"] = stats.sizeY;
            e["speed"] = stats.speed;
            e["damage"] = stats.damage;
            e["canShoot"] = stats.canShoot;
            e["colorR"] = stats.color.r;
            e["colorG"] = stats.color.g;
            e["colorB"] = stats.color.b;
            if (stats.canShoot) {
                e["shootCooldown"] = stats.shootCooldown;
                e["projectileSpeed"] = stats.projectileSpeed;
                e["projectileRange"] = stats.projectileRange;
                e["shootRange"] = stats.shootRange;
            }
        } else if (auto* flying = dynamic_cast<FlyingEnemy*>(enemy.get())) {
            e["type"] = "flying";
            e["patrolDistance"] = flying->getPatrolDistance();
            e["horizontalPatrol"] = true;
            const EnemyStats& stats = enemy->getStats();
            e["maxHP"] = stats.maxHP;
            e["sizeX"] = stats.sizeX;
            e["sizeY"] = stats.sizeY;
            e["speed"] = stats.speed;
            e["damage"] = stats.damage;
            e["canShoot"] = stats.canShoot;
            e["colorR"] = stats.color.r;
            e["colorG"] = stats.color.g;
            e["colorB"] = stats.color.b;
            if (stats.canShoot) {
                e["shootCooldown"] = stats.shootCooldown;
                e["projectileSpeed"] = stats.projectileSpeed;
                e["projectileRange"] = stats.projectileRange;
                e["shootRange"] = stats.shootRange;
            }
        } else if (dynamic_cast<Spike*>(enemy.get())) {
            e["type"] = "spike";
        } else if (auto* flame = dynamic_cast<FlameTrap*>(enemy.get())) {
            e["type"] = "flameTrap";
            const EnemyStats& stats = enemy->getStats();
            e["maxHP"] = stats.maxHP;
            e["sizeX"] = stats.sizeX;
            e["sizeY"] = stats.sizeY;
            e["damage"] = stats.damage;
            e["colorR"] = stats.color.r;
            e["colorG"] = stats.color.g;
            e["colorB"] = stats.color.b;
            e["direction"] = flameDirToString(flame->getDirection());
            e["activeDuration"] = flame->getActiveDuration();
            e["inactiveDuration"] = flame->getInactiveDuration();
            e["shotInterval"] = flame->getShotInterval();
            e["projectileSpeed"] = flame->getProjectileSpeed();
            e["projectileRange"] = flame->getProjectileRange();
        } else if (auto* rotating = dynamic_cast<RotatingTrap*>(enemy.get())) {
            e["type"] = "rotatingTrap";
            const EnemyStats& stats = enemy->getStats();
            e["maxHP"] = stats.maxHP;
            e["sizeX"] = stats.sizeX;
            e["sizeY"] = stats.sizeY;
            e["damage"] = stats.damage;
            e["colorR"] = stats.color.r;
            e["colorG"] = stats.color.g;
            e["colorB"] = stats.color.b;
            e["rotationSpeed"] = rotating->getRotationSpeed();
            e["armLength"] = rotating->getArmLength();
            e["armThickness"] = rotating->getArmThickness();
        }
        j["enemies"].push_back(e);
    }

    j["interactiveObjects"] = nlohmann::json::array();
    for (const auto& obj : ctx.interactiveObjects) {
        sf::Vector2f pos = obj->getPosition();
        sf::Vector2f size = obj->getSize();
        nlohmann::json io;
        io["x"] = pos.x;
        io["y"] = pos.y;
        io["width"] = size.x;
        io["height"] = size.y;
        io["id"] = cleanString(obj->getId());
        std::string typeStr = "terminal";
        switch (obj->getType()) {
            case InteractiveType::Terminal: typeStr = "terminal"; break;
            case InteractiveType::Door: typeStr = "door"; break;
            case InteractiveType::Turret: typeStr = "turret"; break;
        }
        io["type"] = typeStr;
        j["interactiveObjects"].push_back(io);
    }

    j["checkpoints"] = nlohmann::json::array();
    for (const auto& checkpoint : ctx.checkpoints) {
        sf::Vector2f pos = checkpoint->getPosition();
        nlohmann::json cp;
        cp["x"] = pos.x;
        cp["y"] = pos.y;
        cp["id"] = cleanString(checkpoint->getId());
        j["checkpoints"].push_back(cp);
    }

    if (ctx.currentLevel) {
        j["portals"] = nlohmann::json::array();
        for (const auto& portal : ctx.currentLevel->portals) {
            nlohmann::json p;
            p["x"] = portal.x;
            p["y"] = portal.y;
            p["width"] = portal.width;
            p["height"] = portal.height;
            std::string cleanTargetLevel = cleanString(portal.targetLevel);
            p["targetLevel"] = cleanTargetLevel;
            std::string cleanSpawnDirection = cleanString(portal.spawnDirection);
            p["spawnDirection"] = cleanSpawnDirection;
            if (portal.useCustomSpawn) {
                p["useCustomSpawn"] = true;
                p["customSpawnPos"] = nlohmann::json::array({portal.customSpawnPos.x, portal.customSpawnPos.y});
            } else {
                p["useCustomSpawn"] = false;
            }
            j["portals"].push_back(p);
        }
    }

    std::ofstream outFile(savePath);
    if (!outFile.is_open()) {
        std::cout << "Erreur: Impossible d'ecrire dans " << savePath << "\n";
        std::cout << "Chemin original: " << ctx.currentLevelPath << "\n";
        return;
    }

    outFile << j.dump(2);
    outFile.close();

    std::cout << "Niveau sauvegarde dans " << savePath << " (" << ctx.platforms.size() << " plateformes)\n";
    std::cout << "Verifie le fichier: " << savePath << "\n";

    resetState();
    setSaveMessage("Niveau sauvegarde !", sf::Color::Green);
    return;

fallback_save:
#else
    // Fallback: manual JSON writing
#endif
    std::ifstream inFile(savePath);
    if (!inFile.is_open()) {
        std::cout << "Erreur: Impossible d'ouvrir " << savePath << " pour sauvegarde\n";
        setSaveMessage("Erreur: Fichier introuvable", sf::Color::Red);
        return;
    }

    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string content = buffer.str();
    inFile.close();

    if (savePath != ctx.currentLevelPath && savePath.find("PlatformerGame/") == std::string::npos) {
        size_t buildPos = savePath.find("build/");
        if (buildPos != std::string::npos) {
            size_t assetsPos = savePath.find("assets/");
            if (assetsPos != std::string::npos) {
                savePath = "PlatformerGame/" + savePath.substr(assetsPos);
            }
        }
    }

    auto replaceArray = [&](const std::string& key, const std::string& replacement) {
        size_t start = content.find("\"" + key + "\"");
        if (start == std::string::npos) return false;
        size_t arrayStart = content.find("[", start);
        if (arrayStart == std::string::npos) return false;
        size_t arrayEnd = arrayStart;
        int depth = 0;
        do {
            char c = content[arrayEnd];
            if (c == '[') depth++;
            else if (c == ']') depth--;
            arrayEnd++;
        } while (arrayEnd < content.size() && depth > 0);
        content.replace(arrayStart, arrayEnd - arrayStart, replacement);
        return true;
    };

    std::stringstream platformsStream;
    platformsStream << "[\n";
    for (size_t i = 0; i < ctx.platforms.size(); ++i) {
        sf::FloatRect bounds = ctx.platforms[i]->getBounds();
        std::string typeStr = (ctx.platforms[i]->getType() == Platform::Type::EndFloor) ? "endfloor" : "floor";
        platformsStream << "    { \"x\": " << static_cast<int>(bounds.left)
                        << ", \"y\": " << static_cast<int>(bounds.top)
                        << ", \"width\": " << static_cast<int>(bounds.width)
                        << ", \"height\": " << static_cast<int>(bounds.height)
                        << ", \"type\": \"" << typeStr << "\" }";
        if (i < ctx.platforms.size() - 1) platformsStream << ",";
        platformsStream << "\n";
    }
    platformsStream << "  ]";
    if (!replaceArray("platforms", platformsStream.str())) {
        std::cout << "Erreur: Section platforms non trouvee dans le JSON\n";
        setSaveMessage("Erreur: Format JSON invalide", sf::Color::Red);
        return;
    }

    std::stringstream enemiesStream;
    enemiesStream << "[\n";
    auto flameDirToStringFallback = [](FlameDirection dir) {
        switch (dir) {
            case FlameDirection::Left:  return "left";
            case FlameDirection::Right: return "right";
            case FlameDirection::Up:    return "up";
            case FlameDirection::Down:  return "down";
        }
        return "right";
    };
    for (size_t i = 0; i < ctx.enemies.size(); ++i) {
        Enemy* enemy = ctx.enemies[i].get();
        if (!enemy) continue; // Skip null enemies
        sf::Vector2f pos = enemy->getPosition();
        const EnemyStats& stats = enemy->getStats();
        enemiesStream << "    { \"x\": " << static_cast<int>(pos.x)
                      << ", \"y\": " << static_cast<int>(pos.y);
        if (auto* patrol = dynamic_cast<PatrolEnemy*>(enemy)) {
            enemiesStream << ", \"type\": \"patrol\", \"patrolDistance\": " << static_cast<int>(patrol->getPatrolDistance())
                          << ", \"maxHP\": " << stats.maxHP
                          << ", \"sizeX\": " << static_cast<int>(stats.sizeX)
                          << ", \"sizeY\": " << static_cast<int>(stats.sizeY)
                          << ", \"speed\": " << static_cast<int>(stats.speed)
                          << ", \"damage\": " << stats.damage
                          << ", \"canShoot\": " << (stats.canShoot ? "true" : "false")
                          << ", \"colorR\": " << static_cast<int>(stats.color.r)
                          << ", \"colorG\": " << static_cast<int>(stats.color.g)
                          << ", \"colorB\": " << static_cast<int>(stats.color.b);
            if (stats.canShoot) {
                enemiesStream << ", \"shootCooldown\": " << stats.shootCooldown
                              << ", \"projectileSpeed\": " << static_cast<int>(stats.projectileSpeed)
                              << ", \"projectileRange\": " << static_cast<int>(stats.projectileRange)
                              << ", \"shootRange\": " << static_cast<int>(stats.shootRange);
            }
        } else if (auto* flying = dynamic_cast<FlyingEnemy*>(enemy)) {
            enemiesStream << ", \"type\": \"flying\", \"patrolDistance\": " << static_cast<int>(flying->getPatrolDistance()) << ", \"horizontalPatrol\": true"
                          << ", \"maxHP\": " << stats.maxHP
                          << ", \"sizeX\": " << static_cast<int>(stats.sizeX)
                          << ", \"sizeY\": " << static_cast<int>(stats.sizeY)
                          << ", \"speed\": " << static_cast<int>(stats.speed)
                          << ", \"damage\": " << stats.damage
                          << ", \"canShoot\": " << (stats.canShoot ? "true" : "false")
                          << ", \"colorR\": " << static_cast<int>(stats.color.r)
                          << ", \"colorG\": " << static_cast<int>(stats.color.g)
                          << ", \"colorB\": " << static_cast<int>(stats.color.b);
            if (stats.canShoot) {
                enemiesStream << ", \"shootCooldown\": " << stats.shootCooldown
                              << ", \"projectileSpeed\": " << static_cast<int>(stats.projectileSpeed)
                              << ", \"projectileRange\": " << static_cast<int>(stats.projectileRange)
                              << ", \"shootRange\": " << static_cast<int>(stats.shootRange);
            }
        } else if (dynamic_cast<Spike*>(enemy)) {
            enemiesStream << ", \"type\": \"spike\"";
        } else if (auto* flame = dynamic_cast<FlameTrap*>(enemy)) {
            enemiesStream << ", \"type\": \"flameTrap\""
                          << ", \"maxHP\": " << stats.maxHP
                          << ", \"sizeX\": " << static_cast<int>(stats.sizeX)
                          << ", \"sizeY\": " << static_cast<int>(stats.sizeY)
                          << ", \"damage\": " << stats.damage
                          << ", \"colorR\": " << static_cast<int>(stats.color.r)
                          << ", \"colorG\": " << static_cast<int>(stats.color.g)
                          << ", \"colorB\": " << static_cast<int>(stats.color.b)
                          << ", \"direction\": \"" << flameDirToStringFallback(flame->getDirection()) << "\""
                          << ", \"activeDuration\": " << flame->getActiveDuration()
                          << ", \"inactiveDuration\": " << flame->getInactiveDuration()
                          << ", \"shotInterval\": " << flame->getShotInterval()
                          << ", \"projectileSpeed\": " << static_cast<int>(flame->getProjectileSpeed())
                          << ", \"projectileRange\": " << static_cast<int>(flame->getProjectileRange());
        } else if (auto* rotating = dynamic_cast<RotatingTrap*>(enemy)) {
            enemiesStream << ", \"type\": \"rotatingTrap\""
                          << ", \"maxHP\": " << stats.maxHP
                          << ", \"sizeX\": " << static_cast<int>(stats.sizeX)
                          << ", \"sizeY\": " << static_cast<int>(stats.sizeY)
                          << ", \"damage\": " << stats.damage
                          << ", \"colorR\": " << static_cast<int>(stats.color.r)
                          << ", \"colorG\": " << static_cast<int>(stats.color.g)
                          << ", \"colorB\": " << static_cast<int>(stats.color.b)
                          << ", \"rotationSpeed\": " << static_cast<int>(rotating->getRotationSpeed())
                          << ", \"armLength\": " << static_cast<int>(rotating->getArmLength())
                          << ", \"armThickness\": " << static_cast<int>(rotating->getArmThickness());
        }
        enemiesStream << " }";
        if (i < ctx.enemies.size() - 1) enemiesStream << ",";
        enemiesStream << "\n";
    }
    enemiesStream << "  ]";
    replaceArray("enemies", enemiesStream.str());

    std::stringstream interactStream;
    interactStream << "[\n";
    for (size_t i = 0; i < ctx.interactiveObjects.size(); ++i) {
        auto& obj = ctx.interactiveObjects[i];
        sf::Vector2f pos = obj->getPosition();
        sf::Vector2f size = obj->getSize();
        std::string typeStr = "terminal";
        switch (obj->getType()) {
            case InteractiveType::Terminal: typeStr = "terminal"; break;
            case InteractiveType::Door: typeStr = "door"; break;
            case InteractiveType::Turret: typeStr = "turret"; break;
        }
        interactStream << "    { \"type\": \"" << typeStr << "\", \"x\": " << static_cast<int>(pos.x)
                       << ", \"y\": " << static_cast<int>(pos.y)
                       << ", \"width\": " << static_cast<int>(size.x)
                       << ", \"height\": " << static_cast<int>(size.y)
                       << ", \"id\": \"" << cleanString(obj->getId()) << "\" }";
        if (i < ctx.interactiveObjects.size() - 1) interactStream << ",";
        interactStream << "\n";
    }
    interactStream << "  ]";
    replaceArray("interactiveObjects", interactStream.str());

    std::stringstream checkpointsStream;
    checkpointsStream << "[\n";
    for (size_t i = 0; i < ctx.checkpoints.size(); ++i) {
        sf::Vector2f pos = ctx.checkpoints[i]->getPosition();
        checkpointsStream << "    { \"x\": " << static_cast<int>(pos.x)
                          << ", \"y\": " << static_cast<int>(pos.y)
                          << ", \"id\": \"" << cleanString(ctx.checkpoints[i]->getId()) << "\" }";
        if (i < ctx.checkpoints.size() - 1) checkpointsStream << ",";
        checkpointsStream << "\n";
    }
    checkpointsStream << "  ]";
    replaceArray("checkpoints", checkpointsStream.str());

    if (ctx.currentLevel) {
        std::stringstream portalsStream;
        portalsStream << "[\n";
        for (size_t i = 0; i < ctx.currentLevel->portals.size(); ++i) {
            const auto& portal = ctx.currentLevel->portals[i];
            std::string cleanTargetLevel = cleanString(portal.targetLevel);
            std::string cleanSpawnDirection = cleanString(portal.spawnDirection);
            portalsStream << "    {\n"
                          << "      \"x\": " << static_cast<int>(portal.x) << ",\n"
                          << "      \"y\": " << static_cast<int>(portal.y) << ",\n"
                          << "      \"width\": " << static_cast<int>(portal.width) << ",\n"
                          << "      \"height\": " << static_cast<int>(portal.height) << ",\n"
                          << "      \"targetLevel\": \"" << cleanTargetLevel << "\",\n"
                          << "      \"spawnDirection\": \"" << cleanSpawnDirection << "\"";
            if (portal.useCustomSpawn) {
                portalsStream << ",\n      \"useCustomSpawn\": true,\n"
                              << "      \"customSpawnPos\": [" << static_cast<int>(portal.customSpawnPos.x)
                              << ", " << static_cast<int>(portal.customSpawnPos.y) << "]";
            } else {
                portalsStream << ",\n      \"useCustomSpawn\": false";
            }
            portalsStream << "\n    }";
            if (i < ctx.currentLevel->portals.size() - 1) portalsStream << ",";
            portalsStream << "\n";
        }
        portalsStream << "  ]";
        replaceArray("portals", portalsStream.str());
    }

    std::ofstream outFile(savePath);
    if (!outFile.is_open()) {
        std::cout << "Erreur: Impossible d'ecrire dans " << savePath << "\n";
        setSaveMessage("Erreur: Ecriture impossible", sf::Color::Red);
        return;
    }

    outFile << content;
    outFile.close();

    std::cout << "Niveau sauvegarde (fallback) dans " << savePath << " (" << ctx.platforms.size() << " plateformes)\n";
    std::cout << "Verifie le fichier: " << savePath << "\n";

    resetState();
    setSaveMessage("Niveau sauvegarde !", sf::Color::Green);
}

EnemyStats EditorController::getPresetStats(EnemyPresetType preset) const {
    switch (preset) {
        case EnemyPresetType::Basic:
            return EnemyPresets::Basic();
        case EnemyPresetType::Medium:
            return EnemyPresets::Medium();
        case EnemyPresetType::Strong:
            return EnemyPresets::Strong();
        case EnemyPresetType::Shooter:
            return EnemyPresets::Shooter();
        case EnemyPresetType::FastShooter:
            return EnemyPresets::FastShooter();
        case EnemyPresetType::Boss:
            return EnemyPresets::Boss();
        case EnemyPresetType::Fast:
            return EnemyPresets::Fast();
        case EnemyPresetType::FlyingBasic:
            return EnemyPresets::FlyingBasic();
        case EnemyPresetType::FlyingShooter:
            return EnemyPresets::FlyingShooter();
        case EnemyPresetType::FlameHorizontal:
            return EnemyPresets::FlameHorizontal();
        case EnemyPresetType::FlameVertical:
            return EnemyPresets::FlameVertical();
        case EnemyPresetType::RotatingSlow:
            return EnemyPresets::RotatingSlow();
        case EnemyPresetType::RotatingFast:
            return EnemyPresets::RotatingFast();
        default:
            return EnemyPresets::Basic();
    }
}

std::string EditorController::getPresetName(EnemyPresetType preset) const {
    switch (preset) {
        case EnemyPresetType::Basic:
            return "Basic";
        case EnemyPresetType::Medium:
            return "Medium";
        case EnemyPresetType::Strong:
            return "Strong";
        case EnemyPresetType::Shooter:
            return "Shooter";
        case EnemyPresetType::FastShooter:
            return "FastShooter";
        case EnemyPresetType::Boss:
            return "Boss";
        case EnemyPresetType::Fast:
            return "Fast";
        case EnemyPresetType::FlyingBasic:
            return "FlyingBasic";
        case EnemyPresetType::FlyingShooter:
            return "FlyingShooter";
        case EnemyPresetType::FlameHorizontal:
            return "FlameHorizontal";
        case EnemyPresetType::FlameVertical:
            return "FlameVertical";
        case EnemyPresetType::RotatingSlow:
            return "RotatingSlow";
        case EnemyPresetType::RotatingFast:
            return "RotatingFast";
        default:
            return "Basic";
    }
}

void EditorController::applyPresetToEnemy(Enemy* enemy, EnemyPresetType preset, EditorContext& ctx) {
    if (!enemy || selectedEnemyIndex < 0 || selectedEnemyIndex >= static_cast<int>(ctx.enemies.size())) {
        return;
    }
    
    applyPresetDefaults(preset);
    EnemyStats newStats = getPresetStats(preset);
    
    // Get current enemy properties
    sf::Vector2f pos = enemy->getPosition();
    EnemyType enemyType = enemy->getType();
    
    // For PatrolEnemy, get patrol distance
    float patrolDistance = 100.0f;
    if (enemyType == EnemyType::Patrol || enemyType == EnemyType::Flying) {
        patrolDistance = enemy->getPatrolDistance();
    }
    
    // For FlyingEnemy, check if it's horizontal or vertical
    bool isHorizontal = true;
    if (auto* flyingEnemy = dynamic_cast<FlyingEnemy*>(enemy)) {
        // Check if it has vertical bounds set (non-zero)
        if (flyingEnemy->getTopBound() != 0.0f || flyingEnemy->getBottomBound() != 0.0f) {
            isHorizontal = false;
        }
    }
    
    // Validate preset for enemy type
    if (enemyType == EnemyType::Patrol) {
        // Only use ground presets for PatrolEnemy
        if (preset == EnemyPresetType::FlyingBasic || preset == EnemyPresetType::FlyingShooter) {
            newStats = EnemyPresets::Basic();
        }
    } else if (enemyType == EnemyType::Flying) {
        // Use flying presets for FlyingEnemy
        if (preset != EnemyPresetType::FlyingBasic && preset != EnemyPresetType::FlyingShooter) {
            newStats = EnemyPresets::FlyingBasic();
        }
    }
    
    // Recreate enemy with new stats
    std::unique_ptr<Enemy> newEnemy;
    if (enemyType == EnemyType::Patrol) {
        newEnemy = std::make_unique<PatrolEnemy>(pos.x, pos.y, patrolDistance, newStats);
    } else if (enemyType == EnemyType::Flying) {
        newEnemy = std::make_unique<FlyingEnemy>(pos.x, pos.y, patrolDistance, isHorizontal, newStats);
    } else if (enemyType == EnemyType::FlameTrap) {
        if (preset != EnemyPresetType::FlameHorizontal && preset != EnemyPresetType::FlameVertical) {
            newStats = EnemyPresets::FlameHorizontal();
        }
        auto flame = std::make_unique<FlameTrap>(pos.x, pos.y, newStats);
        flame->setDirection(currentFlameDirection);
        flame->setActiveDuration(currentFlameActive);
        flame->setInactiveDuration(currentFlameInactive);
        flame->setShotInterval(currentFlameInterval);
        flame->setProjectileSpeed(currentFlameProjectileSpeed);
        flame->setProjectileRange(currentFlameProjectileRange);
        newEnemy = std::move(flame);
    } else if (enemyType == EnemyType::RotatingTrap) {
        if (preset != EnemyPresetType::RotatingSlow && preset != EnemyPresetType::RotatingFast) {
            newStats = EnemyPresets::RotatingSlow();
        }
        auto trap = std::make_unique<RotatingTrap>(pos.x, pos.y, newStats);
        trap->setRotationSpeed(currentRotatingSpeed);
        trap->setArmLength(currentRotatingArmLength);
        trap->setArmThickness(currentRotatingArmThickness);
        newEnemy = std::move(trap);
    } else {
        // For Stationary (Spike), we can't change preset easily
        return;
    }
    
    // Replace the enemy
    ctx.enemies[selectedEnemyIndex] = std::move(newEnemy);
}

void EditorController::applyPresetDefaults(EnemyPresetType preset) {
    switch (preset) {
        case EnemyPresetType::FlameHorizontal:
            currentFlameDirection = FlameDirection::Right;
            currentFlameActive = 1.5f;
            currentFlameInactive = 1.5f;
            currentFlameInterval = 0.2f;
            currentFlameProjectileSpeed = 350.0f;
            currentFlameProjectileRange = 450.0f;
            break;
        case EnemyPresetType::FlameVertical:
            currentFlameDirection = FlameDirection::Up;
            currentFlameActive = 1.5f;
            currentFlameInactive = 1.5f;
            currentFlameInterval = 0.2f;
            currentFlameProjectileSpeed = 350.0f;
            currentFlameProjectileRange = 450.0f;
            break;
        case EnemyPresetType::RotatingSlow:
            currentRotatingSpeed = 100.0f;
            currentRotatingArmLength = 110.0f;
            currentRotatingArmThickness = 18.0f;
            break;
        case EnemyPresetType::RotatingFast:
            currentRotatingSpeed = 180.0f;
            currentRotatingArmLength = 130.0f;
            currentRotatingArmThickness = 20.0f;
            break;
        default:
            break;
    }
}

