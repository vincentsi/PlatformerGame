#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class Checkpoint {
public:
    Checkpoint(float x, float y, const std::string& id);
    ~Checkpoint() = default;

    void update(float dt);
    void draw(sf::RenderWindow& window);

    // Check if player is touching this checkpoint
    bool isPlayerInside(const sf::FloatRect& playerBounds) const;

    void activate();
    bool isActivated() const { return activated; }

    sf::Vector2f getSpawnPosition() const;
    std::string getId() const { return id; }

private:
    sf::RectangleShape shape;
    sf::Vector2f position;
    std::string id;
    bool activated;

    // Visual feedback
    float pulseTimer;
    sf::Color inactiveColor;
    sf::Color activeColor;
};
