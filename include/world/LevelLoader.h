#pragma once

#include <string>
#include <vector>
#include <memory>
#include "world/Platform.h"
#include "world/GoalZone.h"
#include "world/Checkpoint.h"
#include <SFML/Graphics.hpp>

struct CameraZone {
    float minX, maxX;
    float minY, maxY;
};

struct LevelData {
    std::string name;
    sf::Vector2f startPosition;
    std::vector<std::unique_ptr<Platform>> platforms;
    std::vector<std::unique_ptr<Checkpoint>> checkpoints;
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
