#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include "effects/Particle.h"

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem() = default;

    void update(float dt);
    void draw(sf::RenderWindow& window);

    // Emit different types of particles
    void emitJump(const sf::Vector2f& position);
    void emitLanding(const sf::Vector2f& position);
    void emitDeath(const sf::Vector2f& position);
    void emitVictory(const sf::Vector2f& position);
    void emitGoalGlow(const sf::Vector2f& position, const sf::Vector2f& size);

    void clear();

private:
    void emitParticles(
        const sf::Vector2f& position,
        int count,
        const sf::Color& color,
        float minSpeed,
        float maxSpeed,
        float minLifetime,
        float maxLifetime,
        float minSize,
        float maxSize,
        float angleMin = 0.0f,
        float angleMax = 360.0f
    );

    float randomFloat(float min, float max);

private:
    std::vector<Particle> particles;
    std::mt19937 randomEngine;
    std::uniform_real_distribution<float> distribution;
};
