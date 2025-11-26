#include "systems/CheckpointManager.h"

#include "audio/AudioManager.h"
#include "core/SaveSystem.h"
#include "effects/ParticleSystem.h"
#include "entities/Player.h"
#include "world/Checkpoint.h"
#include "world/LevelLoader.h"

#include <algorithm>
#include <iostream>

CheckpointManager::CheckpointManager(SaveData& saveDataRef,
                                     std::unordered_map<std::string, std::string>& levelCheckpointMap,
                                     std::string& lastLevel,
                                     std::string& lastId,
                                     sf::Vector2f& lastPos)
    : saveData(saveDataRef)
    , levelCheckpoints(levelCheckpointMap)
    , lastCheckpointLevel(lastLevel)
    , lastCheckpointId(lastId)
    , lastCheckpointPos(lastPos) {
}

void CheckpointManager::onCheckpointActivated(const std::string& levelPath,
                                              const std::string& levelId,
                                              Checkpoint& checkpoint,
                                              std::string& activeCheckpointId,
                                              std::vector<std::unique_ptr<Player>>& players,
                                              AudioManager& audioManager,
                                              ParticleSystem& particleSystem) {
    checkpoint.activate();
    activeCheckpointId = checkpoint.getId();

    if (!levelPath.empty()) {
        levelCheckpoints[levelPath] = activeCheckpointId;
    }

    sf::Vector2f cpPos = checkpoint.getSpawnPosition();
    for (auto& player : players) {
        if (player) {
            player->setSpawnPoint(cpPos.x, cpPos.y);
        }
    }

    lastCheckpointLevel = levelPath;
    lastCheckpointId = activeCheckpointId;
    lastCheckpointPos = cpPos;

    audioManager.playSound("checkpoint", 70.0f);
    particleSystem.emitVictory(sf::Vector2f(cpPos.x + 20.0f, cpPos.y + 30.0f));

    saveData.currentLevel = levelIdToNumber(levelId);
    saveData.checkpointX = cpPos.x;
    saveData.checkpointY = cpPos.y;
    size_t copyLen = std::min(activeCheckpointId.length(), static_cast<size_t>(sizeof(saveData.activeCheckpointId) - 1));
    std::memcpy(saveData.activeCheckpointId, activeCheckpointId.c_str(), copyLen);
    saveData.activeCheckpointId[copyLen] = '\0';

    if (SaveSystem::save(saveData)) {
        std::cout << "Game auto-saved at checkpoint: " << activeCheckpointId << "\n";
    }
}

sf::Vector2f CheckpointManager::resolveSpawnPosition(const std::string& levelPath,
                                                     LevelData* level,
                                                     std::vector<std::unique_ptr<Checkpoint>>& checkpoints,
                                                     std::string& activeCheckpointId,
                                                     bool& usedCheckpoint) const {
    sf::Vector2f spawnPos;
    if (!checkpoints.empty() && checkpoints[0]) {
        spawnPos = checkpoints[0]->getSpawnPosition();
    } else if (level && !level->cameraZones.empty()) {
        const auto& zone = level->cameraZones[0];
        spawnPos = sf::Vector2f(zone.minX + 100.0f, zone.minY + 400.0f);
    } else if (level) {
        spawnPos = level->startPosition;
    } else {
        spawnPos = sf::Vector2f(100.0f, 400.0f);
    }
    usedCheckpoint = false;

    auto activateCheckpoint = [&](const std::string& id) -> bool {
        for (auto& checkpoint : checkpoints) {
            if (checkpoint && checkpoint->getId() == id) {
                checkpoint->activate();
                spawnPos = checkpoint->getSpawnPosition();
                activeCheckpointId = id;
                usedCheckpoint = true;
                return true;
            }
        }
        return false;
    };

    auto it = levelCheckpoints.find(levelPath);
    if (it != levelCheckpoints.end() && !it->second.empty()) {
        if (activateCheckpoint(it->second)) {
            return spawnPos;
        }
    }

    if (!lastCheckpointLevel.empty() && !lastCheckpointId.empty() && lastCheckpointLevel == levelPath) {
        if (activateCheckpoint(lastCheckpointId)) {
            return spawnPos;
        }
    }

    activeCheckpointId.clear();
    if (!checkpoints.empty() && checkpoints[0]) {
        spawnPos = checkpoints[0]->getSpawnPosition();
    } else if (level && !level->cameraZones.empty()) {
        const auto& zone = level->cameraZones[0];
        spawnPos = sf::Vector2f(zone.minX + 100.0f, zone.minY + 400.0f);
    }

    return spawnPos;
}

bool CheckpointManager::handleRespawn(const std::string& currentLevelPath,
                                      const std::function<void(const std::string&)>& loadLevelCallback,
                                      std::vector<std::unique_ptr<Player>>& players) const {
    if (lastCheckpointLevel.empty() || lastCheckpointId.empty()) {
        return false;
    }

    if (currentLevelPath != lastCheckpointLevel) {
        loadLevelCallback(lastCheckpointLevel);
        return true;
    }

    for (auto& player : players) {
        if (player) {
            player->setPosition(lastCheckpointPos.x, lastCheckpointPos.y);
            player->setSpawnPoint(lastCheckpointPos.x, lastCheckpointPos.y);
        }
    }
    return false;
}

void CheckpointManager::resetGlobalCheckpoint() {
    lastCheckpointLevel.clear();
    lastCheckpointId.clear();
    lastCheckpointPos = sf::Vector2f(0.0f, 0.0f);
}

int CheckpointManager::levelIdToNumber(const std::string& levelId) const {
    if (levelId == "zone1_level1") return 1;
    if (levelId == "zone1_level2") return 2;
    if (levelId == "zone1_secret") return 2;
    if (levelId == "zone1_level3") return 3;
    if (levelId == "zone1_boss") return 4;
    return saveData.currentLevel;
}

