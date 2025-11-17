#include "world/Platform.h"
#include "graphics/SpriteManager.h"
#include "core/Logger.h"
#include <SFML/Graphics.hpp>

bool Platform::tilesetsLoaded = false;
sf::Texture* Platform::floorTexture = nullptr;
sf::Texture* Platform::endFloorTexture = nullptr;

void Platform::initTilesets() {
    if (!tilesetsLoaded) {
        SpriteManager& sm = SpriteManager::getInstance();
        // Load floor tile
        if (sm.loadTexture("zone1_floor_custom", "assets/tilesets/zone1_floor.png")) {
            floorTexture = sm.getTexture("zone1_floor_custom");
            if (floorTexture) {
                floorTexture->setRepeated(true);
            }
        }
        // Load end floor tile
        if (sm.loadTexture("zone1_endfloor_custom", "assets/tilesets/zone1_endfloor.png")) {
            endFloorTexture = sm.getTexture("zone1_endfloor_custom");
            if (endFloorTexture) {
                endFloorTexture->setRepeated(true);
                Logger::info("End floor texture loaded successfully");
            } else {
                Logger::warning("End floor texture loaded but getTexture returned nullptr");
            }
        } else {
            Logger::warning("Failed to load end floor texture from assets/tilesets/zone1_endfloor.png");
        }
        
        tilesetsLoaded = true;
        Logger::info("Platform tilesets loaded successfully");
    }
}

void Platform::cleanupTilesets() {
    floorTexture = nullptr;
    endFloorTexture = nullptr;
    tilesetsLoaded = false;
}

Platform::Platform(float x, float y, float width, float height, Type type)
    : position(x, y)
    , size(width, height)
    , platformType(type)
{
    // Setup shape for collision bounds (invisible or debug only)
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(position);
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color::Transparent);
    
    updateSprite();
}

void Platform::updateSprite() {
    sf::Texture* texture = getTextureForType(platformType);
    if (texture) {
        sprite.setTexture(*texture);
        const int tileH = static_cast<int>(texture->getSize().y);
        sprite.setTextureRect(sf::IntRect(0, 0, static_cast<int>(size.x), tileH));
        float scaleY = size.y / static_cast<float>(tileH);
        sprite.setScale(1.0f, scaleY);
        sprite.setPosition(position);
    } else {
        // Debug: log when texture is missing
        std::string typeName = (platformType == Type::EndFloor) ? "EndFloor" : "Floor";
        Logger::warning("Texture is nullptr for type " + typeName);
    }
}

sf::Texture* Platform::getTextureForType(Type type) const {
    switch (type) {
        case Type::Floor: return floorTexture;
        case Type::EndFloor: return endFloorTexture;
        default: return floorTexture;
    }
}

void Platform::draw(sf::RenderWindow& window) {
    sf::Texture* texture = getTextureForType(platformType);
    if (texture) {
        // Apply color tint (white for both types since they use different textures)
        sprite.setColor(sf::Color::White);
        window.draw(sprite);
    } else {
        // Fallback to colored shapes based on type
        sf::Color color;
        switch (platformType) {
            case Type::Floor: color = sf::Color(139, 69, 19); break;  // Brown
            case Type::EndFloor: color = sf::Color(160, 82, 45); break;  // Sienna (slightly different brown)
            default: color = sf::Color(139, 69, 19); break;
        }
        shape.setFillColor(color);
        window.draw(shape);
    }
}

sf::FloatRect Platform::getBounds() const {
    return sf::FloatRect(position.x, position.y, size.x, size.y);
}

void Platform::setPosition(float x, float y) {
    position.x = x;
    position.y = y;
    shape.setPosition(position);
    sprite.setPosition(position);
}

void Platform::setSize(float width, float height) {
    size.x = std::max(10.0f, width);
    size.y = std::max(10.0f, height);
    shape.setSize(sf::Vector2f(size.x, size.y));
    updateSprite();
}

void Platform::setType(Type type) {
    platformType = type;
    updateSprite();
}
