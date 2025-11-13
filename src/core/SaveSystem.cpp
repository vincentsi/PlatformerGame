#include "core/SaveSystem.h"
#include "core/Logger.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cstring>

std::string SaveSystem::getSavePath(const std::string& filename) {
    // Save in the same directory as the executable
    return filename;
}

bool SaveSystem::save(const SaveData& data, const std::string& filename) {
    std::string path = getSavePath(filename);
    std::ofstream file(path);

    if (!file.is_open()) {
        Logger::warning("Could not save game data to " + path);
        return false;
    }

    // Write save file in text format (portable)
    file << "SAVE_VERSION:1\n";
    file << "currentLevel:" << data.currentLevel << "\n";
    file << "totalDeaths:" << data.totalDeaths << "\n";
    file << "totalTime:" << data.totalTime << "\n";
    file << "checkpointId:" << data.activeCheckpointId << "\n";
    file << "checkpointX:" << data.checkpointX << "\n";
    file << "checkpointY:" << data.checkpointY << "\n";
    file << "levelsCompleted:";
    for (int i = 0; i < 10; i++) {
        file << (data.levelsCompleted[i] ? "1" : "0");
        if (i < 9) file << ",";
    }
    file << "\n";
    file << "END\n";

    file.close();

    Logger::info("Game saved successfully to " + path);
    return true;
}

bool SaveSystem::load(SaveData& data, const std::string& filename) {
    std::string path = getSavePath(filename);
    std::ifstream file(path);

    if (!file.is_open()) {
        Logger::info("No save file found, starting new game");
        return false;
    }

    // Reset data to default
    data = SaveData();

    std::string line;
    int version = 0;
    bool validFormat = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Check for version
        if (line.find("SAVE_VERSION:") == 0) {
            version = std::stoi(line.substr(13));
            validFormat = true;
            continue;
        }

        if (line == "END") break;

        // Parse key:value pairs
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        try {
            if (key == "currentLevel") {
                int level = std::stoi(value);
                if (level < 1) {
                    Logger::warning("Invalid currentLevel: " + value + ", using default 1");
                    level = 1;
                }
                data.currentLevel = level;
            } else if (key == "totalDeaths") {
                int deaths = std::stoi(value);
                if (deaths < 0) {
                    Logger::warning("Invalid totalDeaths: " + value + ", using default 0");
                    deaths = 0;
                }
                data.totalDeaths = deaths;
            } else if (key == "totalTime") {
                float time = std::stof(value);
                if (time < 0.0f) {
                    Logger::warning("Invalid totalTime: " + value + ", using default 0.0");
                    time = 0.0f;
                }
                data.totalTime = time;
            } else if (key == "checkpointId") {
                size_t len = std::min(value.length(), size_t(63));
                std::memcpy(data.activeCheckpointId, value.c_str(), len);
                data.activeCheckpointId[len] = '\0';
            } else if (key == "checkpointX") {
                data.checkpointX = std::stof(value);
            } else if (key == "checkpointY") {
                data.checkpointY = std::stof(value);
            } else if (key == "levelsCompleted") {
                // Parse comma-separated values
                std::stringstream ss(value);
                std::string token;
                int index = 0;
                while (std::getline(ss, token, ',') && index < 10) {
                    data.levelsCompleted[index] = (token == "1");
                    index++;
                }
            }
        } catch (const std::exception& e) {
            Logger::warning("Error parsing save data key '" + key + "': " + e.what());
            // Continue parsing other keys
        }
    }

    file.close();

    if (!validFormat) {
        Logger::warning("Invalid save file format, starting new game");
        return false;
    }

    // Final validation of loaded data (double-check after parsing)
    if (data.currentLevel < 1) {
        Logger::warning("Invalid currentLevel in save, resetting to 1");
        data.currentLevel = 1;
    }
    if (data.totalDeaths < 0) {
        Logger::warning("Invalid totalDeaths in save, resetting to 0");
        data.totalDeaths = 0;
    }
    if (data.totalTime < 0.0f) {
        Logger::warning("Invalid totalTime in save, resetting to 0.0");
        data.totalTime = 0.0f;
    }

    Logger::info("Save loaded: Level " + std::to_string(data.currentLevel) + ", Deaths: " + std::to_string(data.totalDeaths));
    return true;
}

void SaveSystem::deleteSave(const std::string& filename) {
    std::string path = getSavePath(filename);
    if (std::remove(path.c_str()) == 0) {
        Logger::info("Save file deleted: " + path);
    } else {
        Logger::warning("Could not delete save file: " + path);
    }
}

bool SaveSystem::saveExists(const std::string& filename) {
    std::string path = getSavePath(filename);
    std::ifstream file(path);
    return file.good();
}
