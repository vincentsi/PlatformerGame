#pragma once

#include <string>
#include <fstream>

struct SaveData {
    int currentLevel;
    int totalDeaths;
    float totalTime;
    bool levelsCompleted[10]; // Support for up to 10 levels
    char activeCheckpointId[64]; // ID of the last activated checkpoint
    float checkpointX;
    float checkpointY;

    SaveData()
        : currentLevel(1)
        , totalDeaths(0)
        , totalTime(0.0f)
        , checkpointX(0.0f)
        , checkpointY(0.0f)
    {
        for (int i = 0; i < 10; i++) {
            levelsCompleted[i] = false;
        }
        activeCheckpointId[0] = '\0'; // Empty string
    }
};

class SaveSystem {
public:
    static bool save(const SaveData& data, const std::string& filename = "save.dat");
    static bool load(SaveData& data, const std::string& filename = "save.dat");
    static void deleteSave(const std::string& filename = "save.dat");
    static bool saveExists(const std::string& filename = "save.dat");

private:
    static std::string getSavePath(const std::string& filename);
};
