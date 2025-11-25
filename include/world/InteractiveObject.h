#pragma once

#include <SFML/Graphics.hpp>
#include <string>

enum class InteractiveType {
    Terminal,
    Door,
    Turret
};

class InteractiveObject {
public:
    InteractiveObject(float x, float y, float width, float height, InteractiveType type, const std::string& id);
    virtual ~InteractiveObject() = default;

    virtual void update(float dt);
    virtual void draw(sf::RenderWindow& window);

    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const { return position; }
    sf::Vector2f getSize() const { return size; }
    void setPosition(float x, float y);
    InteractiveType getType() const { return type; }
    std::string getId() const { return id; }
    
    bool isActivated() const { return activated; }
    void activate();
    void deactivate();
    
    // Check if player is in interaction range
    bool isPlayerInRange(const sf::FloatRect& playerBounds) const;

protected:
    sf::Vector2f position;
    sf::Vector2f size;
    InteractiveType type;
    std::string id;
    bool activated;
    float interactionRange;
    
    sf::RectangleShape shape;
};

