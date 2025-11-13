#pragma once

#include <string>
#include <vector>
#include <memory>
#include "world/Platform.h"
#include "world/GoalZone.h"
#include "world/Checkpoint.h"
#include "world/InteractiveObject.h"
#include <SFML/Graphics.hpp>

struct CameraZone {
    float minX, maxX;
    float minY, maxY;
};

struct LevelData {
    std::string name;
    std::string levelId;          // Unique ID (e.g., "zone1_level1", "zone2_north")
    int zoneNumber;               // Zone number (1, 2, 3, etc.)
    bool isBossLevel;             // Is this a boss level?
    std::string nextZone;         // Next zone ID if boss completed
    std::vector<std::string> nextLevels;  // Possible next levels (for non-linear progression)
    sf::Vector2f startPosition;
    std::vector<std::unique_ptr<Platform>> platforms;
    std::vector<std::unique_ptr<Checkpoint>> checkpoints;
    std::vector<std::unique_ptr<InteractiveObject>> interactiveObjects;
    std::unique_ptr<GoalZone> goalZone;
    std::vector<CameraZone> cameraZones;
};

class LevelLoader {
public:
    LevelLoader() = default;
    ~LevelLoader() = default;

    // Load level from JSON file
    static std::unique_ptr<LevelData> loadFromFile(const std::string& filepath);

    // Create a default level if file loading fails
    static std::unique_ptr<LevelData> createDefaultLevel();

private:
    // Helper to parse JSON manually (simple key-value parser)
    static std::string trim(const std::string& str);
    static float parseFloat(const std::string& str);
};
