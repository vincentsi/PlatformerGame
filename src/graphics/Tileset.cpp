#include "graphics/Tileset.h"
#include "graphics/SpriteManager.h"
#include "core/Logger.h"

Tileset::Tileset()
    : texture(nullptr)
    , tileWidth(32)
    , tileHeight(32)
    , columns(0)
    , rows(0)
{}

bool Tileset::load(const std::string& textureId, unsigned int tileW, unsigned int tileH) {
    // Get texture from SpriteManager
    SpriteManager& spriteManager = SpriteManager::getInstance();
    texture = spriteManager.getTexture(textureId);
    
    if (!texture) {
        Logger::error("Failed to load tileset texture: " + textureId);
        return false;
    }

    tileWidth = tileW;
    tileHeight = tileH;

    // Calculate tileset dimensions
    sf::Vector2u textureSize = texture->getSize();
    columns = textureSize.x / tileWidth;
    rows = textureSize.y / tileHeight;

    Logger::info("Loaded tileset '" + textureId + "' (" + 
                 std::to_string(columns) + "x" + std::to_string(rows) + " tiles)");

    return true;
}

sf::Sprite Tileset::getTile(unsigned int index) const {
    unsigned int x = index % columns;
    unsigned int y = index / columns;
    return getTile(x, y);
}

sf::Sprite Tileset::getTile(unsigned int x, unsigned int y) const {
    sf::Sprite sprite;
    
    if (texture && x < columns && y < rows) {
        sprite.setTexture(*texture);
        sprite.setTextureRect(sf::IntRect(
            x * tileWidth,
            y * tileHeight,
            tileWidth,
            tileHeight
        ));
    }
    
    return sprite;
}

void Tileset::drawTile(sf::RenderWindow& window, unsigned int index, float worldX, float worldY) const {
    sf::Sprite tile = getTile(index);
    tile.setPosition(worldX, worldY);
    window.draw(tile);
}

