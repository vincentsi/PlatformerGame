#include "world/Camera.h"
#include "core/Config.h"
#include <algorithm>

// Smooth follow camera. Keeps the target centered, clamped to level limits,
// then adds a small shake offset on top for feedback.
Camera::Camera(float width, float height)
    : view(sf::FloatRect(0.0f, 0.0f, width, height))
    , smoothing(Config::CAMERA_SMOOTHING)
    , shakeOffset(0.0f, 0.0f)
    , limitMinX(0.0f)
    , limitMaxX(0.0f)
    , limitMinY(0.0f)
    , limitMaxY(0.0f)
    , hasLimits(false)
{
    view.setCenter(width / 2.0f, height / 2.0f);
}

void Camera::update(const sf::Vector2f& targetPosition, float dt) {
    sf::Vector2f currentCenter = view.getCenter();
    sf::Vector2f desiredCenter = targetPosition;

    // Smooth camera movement (lerp)
    sf::Vector2f newCenter;
    newCenter.x = currentCenter.x + (desiredCenter.x - currentCenter.x) * smoothing;
    newCenter.y = currentCenter.y + (desiredCenter.y - currentCenter.y) * smoothing;

    // Apply limits if set
    if (hasLimits) {
        float halfWidth = view.getSize().x / 2.0f;
        float halfHeight = view.getSize().y / 2.0f;

        newCenter.x = std::max(limitMinX + halfWidth, std::min(newCenter.x, limitMaxX - halfWidth));
        newCenter.y = std::max(limitMinY + halfHeight, std::min(newCenter.y, limitMaxY - halfHeight));
    }

    view.setCenter(newCenter + shakeOffset);
}

void Camera::apply(sf::RenderWindow& window) {
    window.setView(view);
}

void Camera::setShakeOffset(const sf::Vector2f& offset) {
    shakeOffset = offset;
}

void Camera::setLimits(float minX, float maxX, float minY, float maxY) {
    limitMinX = minX;
    limitMaxX = maxX;
    limitMinY = minY;
    limitMaxY = maxY;
    hasLimits = true;
}
