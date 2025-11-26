#pragma once

#include <SFML/System/Vector2.hpp>
#include <memory>
#include <string>
#include <vector>

struct SaveData;
struct LevelData;

struct ResumeInfo {
    bool hasSave = false;
    bool hasCheckpoint = false;
    std::string levelPath;
    std::string checkpointId;
    sf::Vector2f checkpointPos;
    std::unique_ptr<LevelData> levelData;
};

class SaveManager {
public:
    explicit SaveManager(SaveData& saveData);

    bool loadFromDisk();
    ResumeInfo buildResumeInfo(const std::vector<std::string>& candidateLevels) const;

    const SaveData& data() const { return saveData; }

private:
    SaveData& saveData;
};

