#pragma once

#include <SFML/Graphics.hpp>

class Platform {
public:
    enum class Type {
        Floor,      // Default floor tile
        EndFloor    // End floor tile
    };

    Platform(float x, float y, float width, float height, Type type = Type::Floor);
    ~Platform() = default;

    void draw(sf::RenderWindow& window);

    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const { return position; }
    sf::Vector2f getSize() const { return size; }
    Type getType() const { return platformType; }
    void setPosition(float x, float y);
    void setSize(float width, float height);
    void setType(Type type);

    // Initialize tilesets (call once before creating platforms)
    static void initTilesets();
    
    // Cleanup tilesets
    static void cleanupTilesets();

private:
    sf::Vector2f position;
    sf::Vector2f size;
    Type platformType;
    sf::RectangleShape shape; // Keep for collision bounds
    sf::Sprite sprite; // For rendering tile
    static bool tilesetsLoaded;
    static sf::Texture* floorTexture;
    static sf::Texture* endFloorTexture;
    
    void updateSprite();
    sf::Texture* getTextureForType(Type type) const;
};
