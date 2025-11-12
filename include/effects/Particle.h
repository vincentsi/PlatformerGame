#pragma once

#include <SFML/Graphics.hpp>

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float lifetime;      // Current lifetime
    float maxLifetime;   // Maximum lifetime
    float size;

    Particle(const sf::Vector2f& pos, const sf::Vector2f& vel, const sf::Color& col, float life, float sz)
        : position(pos)
        , velocity(vel)
        , color(col)
        , lifetime(life)
        , maxLifetime(life)
        , size(sz)
    {}

    bool isAlive() const {
        return lifetime > 0.0f;
    }

    void update(float dt) {
        position += velocity * dt;
        lifetime -= dt;

        // Fade out over time
        float alpha = (lifetime / maxLifetime) * 255.0f;
        color.a = static_cast<sf::Uint8>(alpha);
    }
};
