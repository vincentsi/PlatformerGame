#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// Structure pour définir une animation
struct Animation {
    std::string name;
    std::vector<sf::IntRect> frames;  // Rectangles de chaque frame dans la texture
    float frameDuration;              // Durée d'une frame en secondes
    bool loop;                        // L'animation boucle-t-elle?
    
    Animation()
        : name("")
        , frameDuration(0.1f)
        , loop(true)
    {}
};

class AnimatedSprite {
public:
    AnimatedSprite();
    ~AnimatedSprite() = default;

    // Set texture (from SpriteManager)
    void setTexture(sf::Texture* texture);

    // Add animation
    void addAnimation(const Animation& animation);

    // Play animation by name
    void play(const std::string& animationName, bool forceRestart = false);

    // Stop current animation
    void stop();

    // Pause/Resume
    void pause();
    void resume();

    // Update animation (call every frame)
    void update(float dt);

    // Draw sprite
    void draw(sf::RenderWindow& window);

    // Getters/Setters
    void setPosition(float x, float y);
    void setPosition(const sf::Vector2f& position);
    sf::Vector2f getPosition() const;
    
    void setScale(float x, float y);
    void setScale(const sf::Vector2f& scale);
    
    void setOrigin(float x, float y);
    void setOrigin(const sf::Vector2f& origin);
    
    void setRotation(float angle);
    
    void setColor(const sf::Color& color);

    // Get current animation name
    std::string getCurrentAnimation() const;

    // Check if animation is finished (for non-looping animations)
    bool isFinished() const;

    // Get sprite bounds (for collisions)
    sf::FloatRect getGlobalBounds() const;
    sf::FloatRect getLocalBounds() const;

private:
    sf::Sprite sprite;
    sf::Texture* texture;
    
    std::unordered_map<std::string, Animation> animations;
    std::string currentAnimationName;
    Animation* currentAnimation;
    
    size_t currentFrameIndex;
    float frameTimer;
    bool playing;
    bool paused;
    bool finished;
};

