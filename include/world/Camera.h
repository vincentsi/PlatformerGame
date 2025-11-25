#pragma once

#include <SFML/Graphics.hpp>

// Follow camera with smoothing, clamped inside world bounds, plus optional shake offset.
class Camera {
public:
    Camera(float width, float height);
    ~Camera() = default;

    void update(const sf::Vector2f& targetPosition, float dt);
    void apply(sf::RenderWindow& window);
    const sf::View& getView() const { return view; }

    void setLimits(float minX, float maxX, float minY, float maxY);
    void setShakeOffset(const sf::Vector2f& offset);

private:
    sf::View view;
    float smoothing;
    sf::Vector2f shakeOffset;
    float limitMinX, limitMaxX;
    float limitMinY, limitMaxY;
    bool hasLimits;
};
