#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class ParallaxLayer {
public:
    ParallaxLayer();
    ~ParallaxLayer() = default;

    // Load layer from texture ID
    bool load(const std::string& textureId, float scrollSpeedFactor);

    // Update layer position based on camera
    void update(const sf::Vector2f& cameraPosition);

    // Draw layer
    void draw(sf::RenderWindow& window);

    // Setters
    void setScrollSpeed(float factor) { scrollSpeedFactor = factor; }
    void setVerticalOffset(float offset) { verticalOffset = offset; }
    void setTint(const sf::Color& color) { sprite.setColor(color); }

private:
    sf::Sprite sprite;
    sf::Texture* texture;
    float scrollSpeedFactor; // 0.0 = static, 1.0 = moves with camera
    float verticalOffset;
    sf::Vector2f initialPosition;
};

