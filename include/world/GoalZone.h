#pragma once

#include "entities/Entity.h"
#include <SFML/Graphics.hpp>

class GoalZone : public Entity {
public:
    GoalZone(float x, float y, float width, float height);
    ~GoalZone() override = default;

    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

    bool isPlayerInside(const sf::FloatRect& playerBounds) const;

private:
    sf::RectangleShape shape;
    float animationTime;
};
