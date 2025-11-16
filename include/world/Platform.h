#pragma once

#include <SFML/Graphics.hpp>

class Platform {
public:
    Platform(float x, float y, float width, float height);
    ~Platform() = default;

    void draw(sf::RenderWindow& window);

    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const { return position; }
    sf::Vector2f getSize() const { return size; }
    void setPosition(float x, float y);

    // Initialize tileset (call once before creating platforms)
    static void initTileset();
    
    // Cleanup tileset
    static void cleanupTileset();

private:
    sf::Vector2f position;
    sf::Vector2f size;
    sf::RectangleShape shape; // Keep for collision bounds
    sf::Sprite sprite; // For rendering tile
    static bool tilesetLoaded;
    static sf::Texture* tileTexture;
};
