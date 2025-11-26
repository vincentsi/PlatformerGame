#pragma once

#include <SFML/System/Vector2.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class AudioManager;
class ParticleSystem;
class Checkpoint;
class Player;
struct LevelData;
struct SaveData;

class CheckpointManager {
public:
    CheckpointManager(SaveData& saveData,
                      std::unordered_map<std::string, std::string>& levelCheckpoints,
                      std::string& lastCheckpointLevel,
                      std::string& lastCheckpointId,
                      sf::Vector2f& lastCheckpointPos);

    void onCheckpointActivated(const std::string& levelPath,
                               const std::string& levelId,
                               Checkpoint& checkpoint,
                               std::string& activeCheckpointId,
                               std::vector<std::unique_ptr<Player>>& players,
                               AudioManager& audioManager,
                               ParticleSystem& particleSystem);

    sf::Vector2f resolveSpawnPosition(const std::string& levelPath,
                                      LevelData* level,
                                      std::vector<std::unique_ptr<Checkpoint>>& checkpoints,
                                      std::string& activeCheckpointId,
                                      bool& usedCheckpoint) const;

    bool handleRespawn(const std::string& currentLevelPath,
                       const std::function<void(const std::string&)>& loadLevelCallback,
                       std::vector<std::unique_ptr<Player>>& players) const;

    void resetGlobalCheckpoint();

private:
    int levelIdToNumber(const std::string& levelId) const;

    SaveData& saveData;
    std::unordered_map<std::string, std::string>& levelCheckpoints;
    std::string& lastCheckpointLevel;
    std::string& lastCheckpointId;
    sf::Vector2f& lastCheckpointPos;
};

