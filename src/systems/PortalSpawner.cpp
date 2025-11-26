#include "systems/PortalSpawner.h"

#include "world/LevelLoader.h"
#include "world/Platform.h"
#include "core/Config.h"

#include <algorithm>
#include <cctype>

namespace {

std::string cleanDirectionString(const std::string& direction) {
    std::string cleaned = direction;
    auto isTrimChar = [](char c) {
        return c == '"' || c == '\'' || std::isspace(static_cast<unsigned char>(c));
    };

    while (!cleaned.empty() && isTrimChar(cleaned.front())) {
        cleaned.erase(cleaned.begin());
    }
    while (!cleaned.empty() && isTrimChar(cleaned.back())) {
        cleaned.pop_back();
    }

    std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return cleaned;
}

sf::Vector2f defaultSpawnFromLevel(const LevelData* level) {
    if (!level) {
        return sf::Vector2f(100.0f, 400.0f);
    }

    if (!level->cameraZones.empty()) {
        const auto& zone = level->cameraZones[0];
        return sf::Vector2f(zone.minX + 100.0f, zone.minY + 400.0f);
    }

    return level->startPosition;
}

} // namespace

PortalSpawnResult PortalSpawner::computeSpawn(const std::string& pendingDirection,
                                              bool useCustomSpawn,
                                              const sf::Vector2f& customSpawnPos,
                                              const LevelData* level,
                                              const std::vector<std::unique_ptr<Platform>>& platforms) {
    PortalSpawnResult result;

    if (useCustomSpawn) {
        result.position = customSpawnPos;
        result.usedPortal = true;
        return result;
    }

    std::string direction = cleanDirectionString(pendingDirection);
    if (direction.empty() || direction == "default") {
        result.position = defaultSpawnFromLevel(level);
        return result;
    }

    if (!level || level->cameraZones.empty()) {
        result.position = defaultSpawnFromLevel(level);
        return result;
    }

    const auto& zone = level->cameraZones[0];
    sf::Vector2f spawnPos = sf::Vector2f(zone.minX + 100.0f, zone.minY + 400.0f);

    if (direction == "lefttop" || direction == "leftbottom") {
        spawnPos.x = zone.minX + 40.0f;
        float bestY = (direction == "lefttop") ? 10000.0f : -10000.0f;
        bool foundPlatform = false;

        for (const auto& platform : platforms) {
            if (!platform) continue;
            sf::FloatRect bounds = platform->getBounds();
            if (spawnPos.x >= bounds.left - 50.0f && spawnPos.x <= bounds.left + bounds.width + 50.0f) {
                float platformTop = bounds.top;
                if (direction == "lefttop") {
                    if (platformTop < bestY || !foundPlatform) {
                        bestY = platformTop - 60.0f;
                        foundPlatform = true;
                    }
                } else {
                    if (platformTop > bestY) {
                        bestY = platformTop - 60.0f;
                        foundPlatform = true;
                    }
                }
            }
        }

        if (!foundPlatform) {
            spawnPos.y = defaultSpawnFromLevel(level).y;
        } else {
            spawnPos.y = bestY;
        }

        result.position = spawnPos;
        result.usedPortal = true;
        return result;
    }

    if (direction == "righttop" || direction == "rightbottom") {
        float rightmostPlatform = zone.minX;
        float bestY = (direction == "righttop") ? 10000.0f : -10000.0f;
        bool foundPlatform = false;
        float chosenLeft = zone.minX;
        float chosenRight = zone.minX;
        float chosenTop = zone.maxY;

        for (const auto& platform : platforms) {
            if (!platform) continue;
            sf::FloatRect bounds = platform->getBounds();
            float platformRight = bounds.left + bounds.width;

            if (platformRight > rightmostPlatform) {
                rightmostPlatform = platformRight;
            }

            if (platformRight >= zone.maxX - 200.0f) {
                float platformTop = bounds.top;
                if (direction == "righttop") {
                    if (platformTop < bestY || !foundPlatform) {
                        bestY = platformTop - 60.0f;
                        foundPlatform = true;
                        chosenLeft = bounds.left;
                        chosenRight = platformRight;
                        chosenTop = bounds.top;
                    }
                } else {
                    if (platformTop > bestY) {
                        bestY = platformTop - 60.0f;
                        foundPlatform = true;
                        chosenLeft = bounds.left;
                        chosenRight = platformRight;
                        chosenTop = bounds.top;
                    }
                }
            }
        }

        spawnPos.x = rightmostPlatform - 30.0f;
        if (spawnPos.x < zone.minX + 20.0f) {
            spawnPos.x = zone.minX + 20.0f;
        }

        if (!foundPlatform) {
            for (const auto& platform : platforms) {
                if (!platform) continue;
                sf::FloatRect bounds = platform->getBounds();
                float platformRight = bounds.left + bounds.width;

                if (spawnPos.x >= bounds.left - 50.0f && spawnPos.x <= platformRight + 50.0f) {
                    float platformTop = bounds.top;
                    if (direction == "righttop") {
                        if (platformTop < bestY || !foundPlatform) {
                            bestY = platformTop - 60.0f;
                            foundPlatform = true;
                            if (spawnPos.x > platformRight) {
                                spawnPos.x = platformRight - 30.0f;
                            }
                            chosenLeft = bounds.left;
                            chosenRight = platformRight;
                            chosenTop = bounds.top;
                        }
                    } else {
                        if (platformTop > bestY) {
                            bestY = platformTop - 60.0f;
                            foundPlatform = true;
                            if (spawnPos.x > platformRight) {
                                spawnPos.x = platformRight - 30.0f;
                            }
                            chosenLeft = bounds.left;
                            chosenRight = platformRight;
                            chosenTop = bounds.top;
                        }
                    }
                }
            }
        }

        if (!foundPlatform) {
            spawnPos.y = defaultSpawnFromLevel(level).y;
        } else {
            const float playerWidth = Config::PLAYER_WIDTH;
            const float tallestPlayer = Config::PLAYER_HEIGHT + 20.0f;
            const float extraPadding = 40.0f;

            float desiredX = chosenRight - playerWidth - 5.0f - extraPadding;
            float minInside = chosenLeft + 5.0f;
            float maxInside = chosenRight - playerWidth - 5.0f;

            if (minInside > maxInside) {
                float center = (chosenLeft + chosenRight) * 0.5f - playerWidth * 0.5f;
                spawnPos.x = center;
            } else {
                spawnPos.x = std::clamp(desiredX, minInside, maxInside);
            }

            spawnPos.y = chosenTop - tallestPlayer;
        }

        result.position = spawnPos;
        result.usedPortal = true;
        return result;
    }

    result.position = defaultSpawnFromLevel(level);
    return result;
}

