#include "effects/CameraShake.h"

CameraShake::CameraShake()
    : shakeIntensity(0.0f)
    , shakeTimer(0.0f)
    , shakeDuration(0.0f)
    , shakeOffset(0.0f, 0.0f)
    , randomEngine(std::random_device{}())
    , distribution(-1.0f, 1.0f)
{
}

void CameraShake::update(float dt) {
    if (shakeTimer > 0.0f) {
        shakeTimer -= dt;

        // Calculate shake decay (stronger at start, weaker at end)
        float shakeProgress = shakeTimer / shakeDuration;
        float currentIntensity = shakeIntensity * shakeProgress;

        // Generate random offset
        shakeOffset.x = randomFloat(-currentIntensity, currentIntensity);
        shakeOffset.y = randomFloat(-currentIntensity, currentIntensity);

        if (shakeTimer <= 0.0f) {
            shakeOffset = sf::Vector2f(0.0f, 0.0f);
            shakeTimer = 0.0f;
        }
    }
}

sf::Vector2f CameraShake::getOffset() const {
    return shakeOffset;
}

void CameraShake::shake(float intensity, float duration) {
    shakeIntensity = intensity;
    shakeDuration = duration;
    shakeTimer = duration;
}

void CameraShake::shakeLight() {
    shake(5.0f, 0.2f);  // 5px intensity for 0.2 seconds
}

void CameraShake::shakeMedium() {
    shake(12.0f, 0.4f);  // 12px intensity for 0.4 seconds
}

void CameraShake::shakeHeavy() {
    shake(20.0f, 0.6f);  // 20px intensity for 0.6 seconds
}

float CameraShake::randomFloat(float min, float max) {
    return min + distribution(randomEngine) * (max - min) * 0.5f + 0.5f * (max - min);
}
