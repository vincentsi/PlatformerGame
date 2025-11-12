#pragma once

#include <SFML/Graphics.hpp>

class Camera {
public:
    Camera(float width, float height);
    ~Camera() = default;

    void update(const sf::Vector2f& targetPosition, float dt);
    void apply(sf::RenderWindow& window);

    void setLimits(float minX, float maxX, float minY, float maxY);

    // Shake support
    void setShakeOffset(const sf::Vector2f& offset);

private:
    sf::View view;
    float smoothing;
    sf::Vector2f shakeOffset;

    // Camera limits
    float limitMinX, limitMaxX;
    float limitMinY, limitMaxY;
    bool hasLimits;
};
