#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <memory>

class SpriteManager {
public:
    // Singleton pattern
    static SpriteManager& getInstance();

    // Delete copy constructor and assignment
    SpriteManager(const SpriteManager&) = delete;
    SpriteManager& operator=(const SpriteManager&) = delete;

    // Load texture from file
    bool loadTexture(const std::string& id, const std::string& filepath);

    // Get texture by ID (returns nullptr if not found)
    sf::Texture* getTexture(const std::string& id);

    // Unload specific texture
    void unloadTexture(const std::string& id);

    // Unload all textures
    void unloadAll();

    // Check if texture is loaded
    bool hasTexture(const std::string& id) const;

private:
    SpriteManager() = default;
    ~SpriteManager() = default;

    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures;
};

