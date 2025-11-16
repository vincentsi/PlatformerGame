#include "graphics/ParallaxLayer.h"
#include "graphics/SpriteManager.h"
#include "core/Logger.h"

ParallaxLayer::ParallaxLayer()
    : texture(nullptr)
    , scrollSpeedFactor(0.5f)
    , verticalOffset(0.0f)
    , initialPosition(0.0f, 0.0f)
{}

bool ParallaxLayer::load(const std::string& textureId, float scrollSpeed) {
    SpriteManager& spriteManager = SpriteManager::getInstance();
    texture = spriteManager.getTexture(textureId);
    
    if (!texture) {
        Logger::error("Failed to load parallax layer: " + textureId);
        return false;
    }

    sprite.setTexture(*texture);
    scrollSpeedFactor = scrollSpeed;
    
    Logger::info("Loaded parallax layer '" + textureId + "' (scroll: " + 
                 std::to_string(scrollSpeed) + ")");

    return true;
}

void ParallaxLayer::update(const sf::Vector2f& cameraPosition) {
    // Calculate parallax offset
    float offsetX = cameraPosition.x * scrollSpeedFactor;
    float offsetY = cameraPosition.y * scrollSpeedFactor * 0.5f; // Less vertical movement
    
    sprite.setPosition(initialPosition.x - offsetX, initialPosition.y - offsetY + verticalOffset);
}

void ParallaxLayer::draw(sf::RenderWindow& window) {
    // Get current view
    sf::View currentView = window.getView();
    
    // Reset to default view (screen space)
    window.setView(window.getDefaultView());
    
    // Draw background
    window.draw(sprite);
    
    // Restore previous view
    window.setView(currentView);
}

