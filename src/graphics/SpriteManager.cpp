#include "graphics/SpriteManager.h"
#include "core/Logger.h"

SpriteManager& SpriteManager::getInstance() {
    static SpriteManager instance;
    return instance;
}

bool SpriteManager::loadTexture(const std::string& id, const std::string& filepath) {
    // Check if already loaded
    if (hasTexture(id)) {
        Logger::warning("Texture '" + id + "' already loaded");
        return true;
    }

    // Create new texture
    auto texture = std::make_unique<sf::Texture>();
    
    if (!texture->loadFromFile(filepath)) {
        Logger::error("Failed to load texture: " + filepath);
        return false;
    }

    // CRITICAL: Disable smoothing for pixel art
    texture->setSmooth(false);

    // Store texture
    textures[id] = std::move(texture);
    
    Logger::info("Loaded texture '" + id + "' from " + filepath);
    return true;
}

sf::Texture* SpriteManager::getTexture(const std::string& id) {
    auto it = textures.find(id);
    if (it != textures.end()) {
        return it->second.get();
    }
    
    Logger::warning("Texture '" + id + "' not found");
    return nullptr;
}

void SpriteManager::unloadTexture(const std::string& id) {
    auto it = textures.find(id);
    if (it != textures.end()) {
        textures.erase(it);
        Logger::info("Unloaded texture '" + id + "'");
    }
}

void SpriteManager::unloadAll() {
    textures.clear();
    Logger::info("Unloaded all textures");
}

bool SpriteManager::hasTexture(const std::string& id) const {
    return textures.find(id) != textures.end();
}

