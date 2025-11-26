#include "systems/SaveManager.h"

#include "core/SaveSystem.h"
#include "world/LevelLoader.h"
#include "entities/Enemy.h"
#include "world/Checkpoint.h"

#include <algorithm>
#include <cstring>

SaveManager::SaveManager(SaveData& data)
    : saveData(data) {
}

bool SaveManager::loadFromDisk() {
    return SaveSystem::load(saveData);
}

ResumeInfo SaveManager::buildResumeInfo(const std::vector<std::string>& candidateLevels) const {
    ResumeInfo info;
    info.hasSave = true;

    std::string checkpointId = (saveData.activeCheckpointId[0] != '\0')
        ? std::string(saveData.activeCheckpointId)
        : std::string();

    if (!checkpointId.empty()) {
        for (const auto& levelPath : candidateLevels) {
            auto level = LevelLoader::loadFromFile(levelPath);
            if (!level) {
                continue;
            }

            for (const auto& checkpoint : level->checkpoints) {
                if (checkpoint && checkpoint->getId() == checkpointId) {
                    info.hasCheckpoint = true;
                    info.levelPath = levelPath;
                    info.checkpointId = checkpointId;
                    info.checkpointPos = checkpoint->getSpawnPosition();
                    info.levelData = std::move(level);
                    return info;
                }
            }
        }
    }

    // Fallback: use saved level number if checkpoint not found
    int levelNumber = saveData.currentLevel;
    switch (levelNumber) {
        case 1: info.levelPath = "assets/levels/zone1_level1.json"; break;
        case 2: info.levelPath = "assets/levels/zone1_level2.json"; break;
        case 3: info.levelPath = "assets/levels/zone1_level3.json"; break;
        case 4: info.levelPath = "assets/levels/zone1_boss.json"; break;
        default: {
            info.levelPath = "assets/levels/level" + std::to_string(levelNumber) + ".json";
            break;
        }
    }

    info.levelData = LevelLoader::loadFromFile(info.levelPath);
    if (!info.levelData) {
        info.levelPath = "assets/levels/zone1_level1.json";
        info.levelData = LevelLoader::createDefaultLevel();
    }

    return info;
}

