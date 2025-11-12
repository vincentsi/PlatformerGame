#include "core/SaveSystem.h"
#include <iostream>

std::string SaveSystem::getSavePath(const std::string& filename) {
    // Save in the same directory as the executable
    return filename;
}

bool SaveSystem::save(const SaveData& data, const std::string& filename) {
    std::string path = getSavePath(filename);
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "Warning: Could not save game data to " << path << "\n";
        return false;
    }

    file.write(reinterpret_cast<const char*>(&data), sizeof(SaveData));
    file.close();

    std::cout << "Game saved successfully\n";
    return true;
}

bool SaveSystem::load(SaveData& data, const std::string& filename) {
    std::string path = getSavePath(filename);
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "No save file found, starting new game\n";
        return false;
    }

    file.read(reinterpret_cast<char*>(&data), sizeof(SaveData));
    file.close();

    std::cout << "Save loaded: Level " << data.currentLevel << "\n";
    return true;
}

void SaveSystem::deleteSave(const std::string& filename) {
    std::string path = getSavePath(filename);
    std::remove(path.c_str());
    std::cout << "Save file deleted\n";
}

bool SaveSystem::saveExists(const std::string& filename) {
    std::string path = getSavePath(filename);
    std::ifstream file(path);
    return file.good();
}
