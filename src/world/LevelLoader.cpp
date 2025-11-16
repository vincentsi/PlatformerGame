#include "world/LevelLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// Optional: use nlohmann/json if available (header-only, single include)
#if __has_include(<nlohmann/json.hpp>)
    #include <nlohmann/json.hpp>
    #define LEVEL_LOADER_HAS_JSON 1
#else
    #define LEVEL_LOADER_HAS_JSON 0
#endif

// Helper to extract value between quotes or after colon
std::string extractValue(const std::string& str, const std::string& key) {
    size_t keyPos = str.find("\"" + key + "\"");
    if (keyPos == std::string::npos) return "";

    size_t colonPos = str.find(":", keyPos);
    if (colonPos == std::string::npos) return "";

    size_t valueStart = str.find_first_not_of(" \t:", colonPos);
    if (valueStart == std::string::npos) return "";

    size_t valueEnd = str.find_first_of(",}", valueStart);
    if (valueEnd == std::string::npos) valueEnd = str.length();

    std::string value = str.substr(valueStart, valueEnd - valueStart);
    // Remove quotes if present
    if (!value.empty() && value.front() == '"') {
        value = value.substr(1);
        if (!value.empty() && value.back() == '"') {
            value = value.substr(0, value.length() - 1);
        }
    }
    return value;
}

std::unique_ptr<LevelData> LevelLoader::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "Warning: Could not open level file: " << filepath << "\n";
        std::cout << "Loading default level instead.\n";
        return createDefaultLevel();
    }

    // Read entire file
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

#if LEVEL_LOADER_HAS_JSON
    // Preferred path: modern, robust JSON parsing via nlohmann/json
    try {
        nlohmann::json j = nlohmann::json::parse(content);

        auto levelData = std::make_unique<LevelData>();

        // Basic metadata
        levelData->name       = j.value("name",      std::string("Loaded Level"));
        levelData->levelId    = j.value("levelId",   std::string{});
        levelData->zoneNumber = j.value("zoneNumber", 1);
        levelData->isBossLevel = j.value("isBossLevel", false);
        levelData->nextZone   = j.value("nextZone",  std::string{});

        // Level graph: nextLevels (array of strings)
        if (j.contains("nextLevels") && j["nextLevels"].is_array()) {
            for (const auto& v : j["nextLevels"]) {
                if (v.is_string()) {
                    levelData->nextLevels.push_back(v.get<std::string>());
                }
            }
        }

        // Start position: [x, y]
        levelData->startPosition = sf::Vector2f(100.0f, 100.0f);
        if (j.contains("startPosition") && j["startPosition"].is_array() && j["startPosition"].size() >= 2) {
            float x = j["startPosition"][0].get<float>();
            float y = j["startPosition"][1].get<float>();
            levelData->startPosition = sf::Vector2f(x, y);
        }

        // Platforms
        if (j.contains("platforms") && j["platforms"].is_array()) {
            for (const auto& p : j["platforms"]) {
                if (!p.is_object()) continue;
                float x = p.value("x", 0.0f);
                float y = p.value("y", 0.0f);
                float w = p.value("width", 0.0f);
                float h = p.value("height", 0.0f);
                levelData->platforms.push_back(std::make_unique<Platform>(x, y, w, h));
            }
        }

        // Checkpoints
        if (j.contains("checkpoints") && j["checkpoints"].is_array()) {
            for (const auto& c : j["checkpoints"]) {
                if (!c.is_object()) continue;
                float x = c.value("x", 0.0f);
                float y = c.value("y", 0.0f);
                std::string id = c.value("id", std::string{});
                if (!id.empty()) {
                    levelData->checkpoints.push_back(std::make_unique<Checkpoint>(x, y, id));
                }
            }
        }

        // Interactive objects (doors, terminals, turrets...)
        if (j.contains("interactiveObjects") && j["interactiveObjects"].is_array()) {
            for (const auto& io : j["interactiveObjects"]) {
                if (!io.is_object()) continue;
                float x = io.value("x", 0.0f);
                float y = io.value("y", 0.0f);
                float w = io.value("width", 0.0f);
                float h = io.value("height", 0.0f);
                std::string typeStr = io.value("type", std::string{});
                std::string id = io.value("id", std::string{});

                if (id.empty()) continue;

                InteractiveType type = InteractiveType::Terminal;
                if (typeStr == "terminal" || typeStr == "Terminal") {
                    type = InteractiveType::Terminal;
                } else if (typeStr == "door" || typeStr == "Door") {
                    type = InteractiveType::Door;
                } else if (typeStr == "turret" || typeStr == "Turret") {
                    type = InteractiveType::Turret;
                }

                levelData->interactiveObjects.push_back(
                    std::make_unique<InteractiveObject>(x, y, w, h, type, id)
                );
            }
        }

        // Goal zone
        if (j.contains("goalZone") && j["goalZone"].is_object()) {
            const auto& g = j["goalZone"];
            float x = g.value("x", 0.0f);
            float y = g.value("y", 0.0f);
            float w = g.value("width", 0.0f);
            float h = g.value("height", 0.0f);
            levelData->goalZone = std::make_unique<GoalZone>(x, y, w, h);
        }

        // Camera zones
        if (j.contains("cameraZones") && j["cameraZones"].is_array()) {
            for (const auto& cz : j["cameraZones"]) {
                if (!cz.is_object()) continue;
                CameraZone zone{};
                zone.minX = cz.value("minX", 0.0f);
                zone.maxX = cz.value("maxX", 0.0f);
                zone.minY = cz.value("minY", 0.0f);
                zone.maxY = cz.value("maxY", 0.0f);
                levelData->cameraZones.push_back(zone);
            }
        }

        // Validation (same policy as legacy path)
        if (levelData->platforms.empty()) {
            std::cout << "Warning: Level has no platforms. Loading default level.\n";
            return createDefaultLevel();
        }

        // Log level load summary (only key info)
        std::cout << "Level loaded (json): " << levelData->name;
        if (!levelData->levelId.empty()) {
            std::cout << " (ID: " << levelData->levelId << ", Zone: " << levelData->zoneNumber << ")";
        }
        std::cout << "\n";

        return levelData;
    } catch (const std::exception& e) {
        std::cout << "Warning: JSON parse failed for level '" << filepath
                  << "': " << e.what() << "\n";
        std::cout << "Falling back to legacy string parser.\n";
    }
#endif // LEVEL_LOADER_HAS_JSON

    // Legacy path: manual string parsing (kept as robust fallback)
    auto levelData = std::make_unique<LevelData>();
    levelData->name = "Loaded Level";
    levelData->levelId = "";
    levelData->zoneNumber = 1;
    levelData->isBossLevel = false;
    levelData->nextZone = "";
    levelData->startPosition = sf::Vector2f(100.0f, 100.0f);

    // Extract name
    std::string nameValue = extractValue(content, "name");
    if (!nameValue.empty()) {
        levelData->name = nameValue;
    }

    // Extract levelId
    std::string levelIdValue = extractValue(content, "levelId");
    if (!levelIdValue.empty()) {
        levelData->levelId = levelIdValue;
    }

    // Extract zoneNumber
    std::string zoneNumberValue = extractValue(content, "zoneNumber");
    if (!zoneNumberValue.empty()) {
        levelData->zoneNumber = static_cast<int>(parseFloat(zoneNumberValue));
    }

    // Extract isBossLevel (boolean - can be true/false without quotes)
    size_t isBossPos = content.find("\"isBossLevel\"");
    if (isBossPos != std::string::npos) {
        size_t colonPos = content.find(":", isBossPos);
        if (colonPos != std::string::npos) {
            size_t valueStart = content.find_first_not_of(" \t:", colonPos);
            if (valueStart != std::string::npos) {
                size_t valueEnd = content.find_first_of(",}", valueStart);
                if (valueEnd == std::string::npos) valueEnd = content.length();
                std::string isBossValue = trim(content.substr(valueStart, valueEnd - valueStart));
                // Remove quotes if present
                if (!isBossValue.empty() && isBossValue.front() == '"') {
                    isBossValue = isBossValue.substr(1);
                    if (!isBossValue.empty() && isBossValue.back() == '"') {
                        isBossValue = isBossValue.substr(0, isBossValue.length() - 1);
                    }
                }
                levelData->isBossLevel = (isBossValue == "true" || isBossValue == "True" || isBossValue == "1");
            }
        }
    }

    // Extract nextZone
    std::string nextZoneValue = extractValue(content, "nextZone");
    if (!nextZoneValue.empty()) {
        levelData->nextZone = nextZoneValue;
    }

    // Extract nextLevels (array of strings)
    size_t nextLevelsStart = content.find("\"nextLevels\"");
    if (nextLevelsStart != std::string::npos) {
        // Find the opening bracket of the array
        size_t arrayStart = content.find("[", nextLevelsStart);
        if (arrayStart != std::string::npos) {
            // Find the matching closing bracket
            int bracketDepth = 1;
            size_t arrayEnd = arrayStart + 1;
            while (arrayEnd < content.length() && bracketDepth > 0) {
                if (content[arrayEnd] == '[') bracketDepth++;
                else if (content[arrayEnd] == ']') bracketDepth--;
                if (bracketDepth > 0) arrayEnd++;
            }
            
            if (arrayEnd < content.length() && bracketDepth == 0) {
                // Extract the array content (between brackets)
                std::string arrayContent = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
                size_t pos = 0;
                // Find all quoted strings in the array
                while ((pos = arrayContent.find("\"", pos)) != std::string::npos) {
                    size_t quoteEnd = arrayContent.find("\"", pos + 1);
                    if (quoteEnd == std::string::npos) break;
                    std::string levelId = arrayContent.substr(pos + 1, quoteEnd - pos - 1);
                    if (!levelId.empty()) {
                        levelData->nextLevels.push_back(levelId);
                        std::cout << "  Parsed nextLevel: " << levelId << "\n";
                    }
                    pos = quoteEnd + 1;
                }
            }
        }
    }

    // Parse startPosition (array format: [x, y])
    size_t startPosStart = content.find("\"startPosition\"");
    if (startPosStart != std::string::npos) {
        size_t arrayStart = content.find("[", startPosStart);
        size_t arrayEnd = content.find("]", arrayStart);
        if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
            std::string arrayContent = content.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
            size_t commaPos = arrayContent.find(",");
            if (commaPos != std::string::npos) {
                std::string xStr = trim(arrayContent.substr(0, commaPos));
                std::string yStr = trim(arrayContent.substr(commaPos + 1));
                if (!xStr.empty() && !yStr.empty()) {
                    float x = parseFloat(xStr);
                    float y = parseFloat(yStr);
                    levelData->startPosition = sf::Vector2f(x, y);
                }
            }
        }
    }

    // Parse platforms - find all {"x": ..., "y": ..., "width": ..., "height": ...}
    size_t platformsStart = content.find("\"platforms\"");
    if (platformsStart != std::string::npos) {
        size_t arrayStart = content.find("[", platformsStart);
        if (arrayStart != std::string::npos) {
            // Find the matching closing bracket (handle nested brackets)
            int bracketDepth = 1;
            size_t platformsEnd = arrayStart + 1;
            while (platformsEnd < content.length() && bracketDepth > 0) {
                if (content[platformsEnd] == '[') bracketDepth++;
                else if (content[platformsEnd] == ']') bracketDepth--;
                if (bracketDepth > 0) platformsEnd++;
            }
            
            if (platformsEnd < content.length() && bracketDepth == 0) {
                std::string platformsSection = content.substr(arrayStart, platformsEnd - arrayStart);

        size_t pos = 0;
        while ((pos = platformsSection.find("{", pos)) != std::string::npos) {
            size_t objEnd = platformsSection.find("}", pos);
            if (objEnd == std::string::npos) break;

            std::string obj = platformsSection.substr(pos, objEnd - pos + 1);

            std::string xVal = extractValue(obj, "x");
            std::string yVal = extractValue(obj, "y");
            std::string wVal = extractValue(obj, "width");
            std::string hVal = extractValue(obj, "height");

            if (!xVal.empty() && !yVal.empty() && !wVal.empty() && !hVal.empty()) {
                float x = parseFloat(xVal);
                float y = parseFloat(yVal);
                float w = parseFloat(wVal);
                float h = parseFloat(hVal);
                levelData->platforms.push_back(std::make_unique<Platform>(x, y, w, h));
            }

            pos = objEnd + 1;
                }
            } else {
                std::cout << "Warning: Could not find end of platforms array\n";
            }
        } else {
            std::cout << "Warning: Could not find start of platforms array\n";
        }
    }

    // Parse checkpoints
    size_t checkpointsStart = content.find("\"checkpoints\"");
    size_t checkpointsEnd = content.find("],", checkpointsStart);
    if (checkpointsEnd == std::string::npos) checkpointsEnd = content.find("]", checkpointsStart);

    if (checkpointsStart != std::string::npos && checkpointsEnd != std::string::npos) {
        std::string checkpointsSection = content.substr(checkpointsStart, checkpointsEnd - checkpointsStart);

        size_t pos = 0;
        while ((pos = checkpointsSection.find("{", pos)) != std::string::npos) {
            size_t objEnd = checkpointsSection.find("}", pos);
            if (objEnd == std::string::npos) break;

            std::string obj = checkpointsSection.substr(pos, objEnd - pos + 1);

            std::string xVal = extractValue(obj, "x");
            std::string yVal = extractValue(obj, "y");
            std::string idVal = extractValue(obj, "id");

            if (!xVal.empty() && !yVal.empty() && !idVal.empty()) {
                float x = parseFloat(xVal);
                float y = parseFloat(yVal);
                levelData->checkpoints.push_back(std::make_unique<Checkpoint>(x, y, idVal));
            }

            pos = objEnd + 1;
        }
    }

    // Parse interactiveObjects (terminals, doors, etc.)
    size_t interactivesStart = content.find("\"interactiveObjects\"");
    size_t interactivesEnd = content.find("],", interactivesStart);
    if (interactivesEnd == std::string::npos) interactivesEnd = content.find("]", interactivesStart);

    if (interactivesStart != std::string::npos && interactivesEnd != std::string::npos) {
        std::string interactivesSection = content.substr(interactivesStart, interactivesEnd - interactivesStart);

        size_t pos = 0;
        while ((pos = interactivesSection.find("{", pos)) != std::string::npos) {
            size_t objEnd = interactivesSection.find("}", pos);
            if (objEnd == std::string::npos) break;

            std::string obj = interactivesSection.substr(pos, objEnd - pos + 1);

            std::string xVal = extractValue(obj, "x");
            std::string yVal = extractValue(obj, "y");
            std::string wVal = extractValue(obj, "width");
            std::string hVal = extractValue(obj, "height");
            std::string typeVal = extractValue(obj, "type");
            std::string idVal = extractValue(obj, "id");

            if (!xVal.empty() && !yVal.empty() && !wVal.empty() && !hVal.empty() && !typeVal.empty() && !idVal.empty()) {
                float x = parseFloat(xVal);
                float y = parseFloat(yVal);
                float w = parseFloat(wVal);
                float h = parseFloat(hVal);
                
                InteractiveType type = InteractiveType::Terminal; // Default
                if (typeVal == "terminal" || typeVal == "Terminal") {
                    type = InteractiveType::Terminal;
                } else if (typeVal == "door" || typeVal == "Door") {
                    type = InteractiveType::Door;
                } else if (typeVal == "turret" || typeVal == "Turret") {
                    type = InteractiveType::Turret;
                }
                
                levelData->interactiveObjects.push_back(
                    std::make_unique<InteractiveObject>(x, y, w, h, type, idVal)
                );
            }

            pos = objEnd + 1;
        }
    }

    // Parse goalZone
    size_t goalStart = content.find("\"goalZone\"");
    if (goalStart != std::string::npos) {
        size_t objStart = content.find("{", goalStart);
        size_t objEnd = content.find("}", objStart);

        if (objStart != std::string::npos && objEnd != std::string::npos) {
            std::string obj = content.substr(objStart, objEnd - objStart + 1);

            std::string xVal = extractValue(obj, "x");
            std::string yVal = extractValue(obj, "y");
            std::string wVal = extractValue(obj, "width");
            std::string hVal = extractValue(obj, "height");

            if (!xVal.empty() && !yVal.empty() && !wVal.empty() && !hVal.empty()) {
                float x = parseFloat(xVal);
                float y = parseFloat(yVal);
                float w = parseFloat(wVal);
                float h = parseFloat(hVal);
                levelData->goalZone = std::make_unique<GoalZone>(x, y, w, h);
            }
        }
    }

    // Parse camera zones
    size_t cameraStart = content.find("\"cameraZones\"");
    size_t cameraEnd = content.find("],", cameraStart);
    if (cameraEnd == std::string::npos) cameraEnd = content.find("]", cameraStart);

    if (cameraStart != std::string::npos && cameraEnd != std::string::npos) {
        std::string cameraSection = content.substr(cameraStart, cameraEnd - cameraStart);

        size_t pos = 0;
        while ((pos = cameraSection.find("{", pos)) != std::string::npos) {
            size_t objEnd = cameraSection.find("}", pos);
            if (objEnd == std::string::npos) break;

            std::string obj = cameraSection.substr(pos, objEnd - pos + 1);

            std::string minXVal = extractValue(obj, "minX");
            std::string maxXVal = extractValue(obj, "maxX");
            std::string minYVal = extractValue(obj, "minY");
            std::string maxYVal = extractValue(obj, "maxY");

            if (!minXVal.empty() && !maxXVal.empty() && !minYVal.empty() && !maxYVal.empty()) {
                CameraZone zone;
                zone.minX = parseFloat(minXVal);
                zone.maxX = parseFloat(maxXVal);
                zone.minY = parseFloat(minYVal);
                zone.maxY = parseFloat(maxYVal);
                levelData->cameraZones.push_back(zone);
            }

            pos = objEnd + 1;
        }
    }

    // Validation
    if (levelData->platforms.empty()) {
        std::cout << "Warning: Level has no platforms. Loading default level.\n";
        return createDefaultLevel();
    }

    // Log level load summary (only key info)
    std::cout << "Level loaded: " << levelData->name;
    if (!levelData->levelId.empty()) {
        std::cout << " (ID: " << levelData->levelId << ", Zone: " << levelData->zoneNumber << ")";
    }
    std::cout << "\n";

    return levelData;
}

std::unique_ptr<LevelData> LevelLoader::createDefaultLevel() {
    auto levelData = std::make_unique<LevelData>();
    levelData->name = "Default Level";
    levelData->levelId = "default";
    levelData->zoneNumber = 1;
    levelData->isBossLevel = false;
    levelData->nextZone = "";
    levelData->startPosition = sf::Vector2f(100.0f, 100.0f);

    // Simple test level
    levelData->platforms.push_back(std::make_unique<Platform>(0.0f, 550.0f, 800.0f, 50.0f));
    levelData->platforms.push_back(std::make_unique<Platform>(900.0f, 500.0f, 200.0f, 20.0f));
    levelData->platforms.push_back(std::make_unique<Platform>(1200.0f, 450.0f, 200.0f, 20.0f));

    levelData->goalZone = std::make_unique<GoalZone>(1300.0f, 370.0f, 80.0f, 80.0f);

    return levelData;
}

std::string LevelLoader::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n,");
    return str.substr(first, last - first + 1);
}

float LevelLoader::parseFloat(const std::string& str) {
    std::string cleaned = trim(str);
    // Remove trailing comma if present
    if (!cleaned.empty() && cleaned.back() == ',') {
        cleaned.pop_back();
    }
    try {
        return std::stof(cleaned);
    } catch (...) {
        return 0.0f;
    }
}
