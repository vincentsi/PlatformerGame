#pragma once

#include <SFML/System/Vector2.hpp>
#include <memory>
#include <string>
#include <vector>

struct LevelData;
class Platform;

struct PortalSpawnResult {
    sf::Vector2f position;
    bool usedPortal = false;
};

class PortalSpawner {
public:
    static PortalSpawnResult computeSpawn(const std::string& pendingDirection,
                                          bool useCustomSpawn,
                                          const sf::Vector2f& customSpawnPos,
                                          const LevelData* level,
                                          const std::vector<std::unique_ptr<Platform>>& platforms);
};

