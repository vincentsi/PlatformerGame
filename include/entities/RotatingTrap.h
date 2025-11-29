#pragma once

#include "entities/Enemy.h"

class RotatingTrap : public Enemy {
public:
    RotatingTrap(float x, float y, const EnemyStats& stats);

    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

    float getRotationSpeed() const { return rotationSpeed; }
    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    void toggleDirection() { rotationSpeed = -rotationSpeed; }

    float getArmLength() const { return armLength; }
    void setArmLength(float value);

    float getArmThickness() const { return armThickness; }
    void setArmThickness(float value);

    float getDamageRadius() const { return boundingRadius; }

    sf::FloatRect getBounds() const override;
    void setPosition(float x, float y) override;
    void setPosition(const sf::Vector2f& pos) override;

private:
    void updateShape();

    float rotationSpeed;
    float angle;
    float armLength;
    float armThickness;
    float boundingRadius;
    sf::Vector2f pivot;
};


