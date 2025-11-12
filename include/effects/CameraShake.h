#pragma once

#include <SFML/Graphics.hpp>
#include <random>

class CameraShake {
public:
    CameraShake();
    ~CameraShake() = default;

    void update(float dt);
    sf::Vector2f getOffset() const;

    // Trigger different shake intensities
    void shake(float intensity, float duration);
    void shakeLight();    // For landing
    void shakeMedium();   // For death
    void shakeHeavy();    // For big events

    bool isShaking() const { return shakeTimer > 0.0f; }

private:
    float randomFloat(float min, float max);

private:
    float shakeIntensity;
    float shakeTimer;
    float shakeDuration;
    sf::Vector2f shakeOffset;

    std::mt19937 randomEngine;
    std::uniform_real_distribution<float> distribution;
};
