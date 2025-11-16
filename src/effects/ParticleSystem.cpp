#include "effects/ParticleSystem.h"
#include <cmath>

ParticleSystem::ParticleSystem()
    : randomEngine(std::random_device{}())
    , distribution(0.0f, 1.0f)
{
    particles.reserve(1000); // Pre-allocate for performance
}

void ParticleSystem::update(float dt) {
    // Update all particles
    for (auto& particle : particles) {
        particle.update(dt);
    }

    // Remove dead particles
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return !p.isAlive(); }),
        particles.end()
    );
}

void ParticleSystem::draw(sf::RenderWindow& window) {
    for (const auto& particle : particles) {
        sf::CircleShape shape(particle.size);
        shape.setPosition(particle.position);
        shape.setFillColor(particle.color);
        shape.setOrigin(particle.size, particle.size);
        window.draw(shape);
    }
}

void ParticleSystem::emitJump(const sf::Vector2f& position) {
    // Small burst of green particles downward when jumping
    emitParticles(
        position,
        8,                              // count
        sf::Color(100, 255, 100),      // light green
        50.0f, 150.0f,                 // speed range
        0.3f, 0.6f,                    // lifetime range
        2.0f, 4.0f,                    // size range
        90.0f, 180.0f                  // angle range (downward)
    );
}

void ParticleSystem::emitLanding(const sf::Vector2f& position) {
    // Dust particles spreading sideways on landing
    emitParticles(
        position,
        12,                             // count
        sf::Color(200, 200, 200),      // gray dust
        80.0f, 200.0f,                 // speed range
        0.4f, 0.8f,                    // lifetime range
        3.0f, 6.0f,                    // size range
        45.0f, 135.0f                  // angle range (sideways spread)
    );
}

void ParticleSystem::emitDeath(const sf::Vector2f& position) {
    // Explosion of red particles in all directions
    emitParticles(
        position,
        30,                             // count
        sf::Color(255, 50, 50),        // bright red
        100.0f, 300.0f,                // speed range
        0.5f, 1.2f,                    // lifetime range
        4.0f, 8.0f,                    // size range
        0.0f, 360.0f                   // angle range (all directions)
    );

    // Add some orange particles for effect
    emitParticles(
        position,
        20,                             // count
        sf::Color(255, 150, 50),       // orange
        80.0f, 250.0f,                 // speed range
        0.4f, 1.0f,                    // lifetime range
        3.0f, 7.0f,                    // size range
        0.0f, 360.0f                   // angle range (all directions)
    );
}

void ParticleSystem::emitVictory(const sf::Vector2f& position) {
    // Golden confetti particles shooting upward
    emitParticles(
        position,
        40,                             // count
        sf::Color(255, 215, 0),        // gold
        150.0f, 400.0f,                // speed range
        1.0f, 2.0f,                    // lifetime range
        4.0f, 8.0f,                    // size range
        -45.0f, 45.0f                  // angle range (upward spray)
    );

    // Add yellow particles
    emitParticles(
        position,
        30,                             // count
        sf::Color(255, 255, 100),      // yellow
        120.0f, 350.0f,                // speed range
        0.8f, 1.8f,                    // lifetime range
        3.0f, 7.0f,                    // size range
        -45.0f, 45.0f                  // angle range (upward spray)
    );
}

void ParticleSystem::emitGoalGlow(const sf::Vector2f& position, const sf::Vector2f& size) {
    // Gentle floating golden particles around goal zone
    for (int i = 0; i < 3; ++i) {
        float xOffset = randomFloat(-size.x / 2.0f, size.x / 2.0f);
        float yOffset = randomFloat(-size.y / 2.0f, size.y / 2.0f);
        sf::Vector2f spawnPos = position + sf::Vector2f(xOffset, yOffset);

        emitParticles(
            spawnPos,
            1,                              // count (one at a time for continuous effect)
            sf::Color(255, 215, 0, 180),   // semi-transparent gold
            10.0f, 30.0f,                  // slow speed
            1.5f, 3.0f,                    // long lifetime
            3.0f, 6.0f,                    // size range
            -90.0f, -45.0f                 // angle range (floating upward)
        );
    }
}

void ParticleSystem::emitKineticWave(const sf::Vector2f& position, const sf::Vector2f& direction, float range) {
    // Calculate angle from direction vector
    const float PI = 3.14159265359f;
    float baseAngle = std::atan2(direction.y, direction.x) * 180.0f / PI; // Convert to degrees
    
    // Main wave particles - blue/cyan particles propagating forward
    // Create multiple waves at different distances to show propagation
    for (int wave = 0; wave < 4; ++wave) {
        float waveDistance = (range / 4.0f) * (wave + 1); // Each wave at 25%, 50%, 75%, 100% of range
        sf::Vector2f wavePos = position + direction * waveDistance;
        
        // Horizontal spread wave (wider spread for visual effect)
        emitParticles(
            wavePos,
            15,                             // count per wave
            sf::Color(100, 200, 255),      // light blue/cyan
            200.0f, 400.0f,                // speed (propagating outward)
            0.15f, 0.3f,                   // short lifetime (quick burst)
            4.0f, 8.0f,                    // size range
            baseAngle - 30.0f,             // angle range (spread from direction)
            baseAngle + 30.0f
        );
        
        // Add some lighter cyan particles for glow effect
        emitParticles(
            wavePos,
            8,                              // count
            sf::Color(150, 230, 255),      // lighter cyan
            150.0f, 300.0f,                // speed
            0.2f, 0.4f,                    // lifetime
            6.0f, 12.0f,                   // larger size
            baseAngle - 45.0f,             // wider spread
            baseAngle + 45.0f
        );
    }
    
    // Initial burst at player position
    emitParticles(
        position,
        20,                                // count
        sf::Color(100, 200, 255),         // light blue/cyan
        300.0f, 500.0f,                   // fast initial burst
        0.2f, 0.4f,                       // lifetime
        5.0f, 10.0f,                      // size range
        baseAngle - 45.0f,                // wide spread from direction
        baseAngle + 45.0f
    );
}

void ParticleSystem::clear() {
    particles.clear();
}

void ParticleSystem::emitParticles(
    const sf::Vector2f& position,
    int count,
    const sf::Color& color,
    float minSpeed,
    float maxSpeed,
    float minLifetime,
    float maxLifetime,
    float minSize,
    float maxSize,
    float angleMin,
    float angleMax
) {
    const float PI = 3.14159265359f;

    for (int i = 0; i < count; ++i) {
        // Random angle in radians
        float angle = randomFloat(angleMin, angleMax) * PI / 180.0f;
        float speed = randomFloat(minSpeed, maxSpeed);

        // Calculate velocity from angle and speed
        sf::Vector2f velocity(
            std::cos(angle) * speed,
            std::sin(angle) * speed
        );

        float lifetime = randomFloat(minLifetime, maxLifetime);
        float size = randomFloat(minSize, maxSize);

        particles.emplace_back(position, velocity, color, lifetime, size);
    }
}

float ParticleSystem::randomFloat(float min, float max) {
    return min + distribution(randomEngine) * (max - min);
}
