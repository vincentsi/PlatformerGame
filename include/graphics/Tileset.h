#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class Tileset {
public:
    Tileset();
    ~Tileset() = default;

    // Load tileset from texture ID and tile size
    bool load(const std::string& textureId, unsigned int tileWidth, unsigned int tileHeight);

    // Get sprite for specific tile index
    sf::Sprite getTile(unsigned int index) const;

    // Get sprite for tile at grid position (x, y) in tileset
    sf::Sprite getTile(unsigned int x, unsigned int y) const;

    // Draw tile at world position
    void drawTile(sf::RenderWindow& window, unsigned int index, float worldX, float worldY) const;

    // Get tile dimensions
    unsigned int getTileWidth() const { return tileWidth; }
    unsigned int getTileHeight() const { return tileHeight; }
    
    // Get tileset dimensions (in tiles)
    unsigned int getColumns() const { return columns; }
    unsigned int getRows() const { return rows; }

private:
    sf::Texture* texture;
    unsigned int tileWidth;
    unsigned int tileHeight;
    unsigned int columns;
    unsigned int rows;
};

