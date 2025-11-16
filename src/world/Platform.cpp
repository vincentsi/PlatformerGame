#include "world/Platform.h"
#include "graphics/SpriteManager.h"
#include "core/Logger.h"
#include <SFML/Graphics.hpp>

bool Platform::tilesetLoaded = false;
sf::Texture* Platform::tileTexture = nullptr;

void Platform::initTileset() {
    if (!tilesetLoaded) {
        SpriteManager& sm = SpriteManager::getInstance();
        // Load custom Zone 1 floor tile (user-provided)
        if (sm.loadTexture("zone1_floor_custom", "assets/tilesets/zone1_floor.png")) {
            tileTexture = sm.getTexture("zone1_floor_custom");
            if (tileTexture) {
                tileTexture->setRepeated(true);
            }
            tilesetLoaded = true;
            Logger::info("Zone 1 floor (custom) tile loaded successfully");
        } else {
            Logger::warning("Failed to load Zone 1 floor custom tile - using default shape");
        }
    }
}

void Platform::cleanupTileset() {
    tileTexture = nullptr;
    tilesetLoaded = false;
}

Platform::Platform(float x, float y, float width, float height)
    : position(x, y)
    , size(width, height)
{
    // Setup shape for collision bounds (invisible or debug only)
    shape.setSize(sf::Vector2f(width, height));
    shape.setPosition(position);
    shape.setFillColor(sf::Color::Transparent); // Make invisible
    shape.setOutlineColor(sf::Color::Transparent);
    
    // Setup sprite for visual rendering
    if (tileTexture) {
        sprite.setTexture(*tileTexture);
        // Repeat horizontally, single tile vertically (stretched to height)
        const int tileW = static_cast<int>(tileTexture->getSize().x);
        const int tileH = static_cast<int>(tileTexture->getSize().y);
        // Texture rect: width = platform width (repeat in X), height = one tile height (no repeat in Y)
        sprite.setTextureRect(sf::IntRect(0, 0, static_cast<int>(size.x), tileH));
        // Scale vertically to match platform height; keep X scale at 1 (repeat covers width)
        float scaleY = size.y / static_cast<float>(tileH);
        sprite.setScale(1.0f, scaleY);
        sprite.setPosition(position);
    }
}

void Platform::draw(sf::RenderWindow& window) {
    if (tileTexture) {
        // Draw tile sprite
        window.draw(sprite);
    } else {
        // Fallback to shape if texture not loaded
        shape.setFillColor(sf::Color(139, 69, 19)); // Brown fallback
        window.draw(shape);
    }
}

sf::FloatRect Platform::getBounds() const {
    return sf::FloatRect(position.x, position.y, size.x, size.y);
}
