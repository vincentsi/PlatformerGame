#include "graphics/AnimatedSprite.h"
#include "core/Logger.h"

AnimatedSprite::AnimatedSprite()
    : texture(nullptr)
    , currentAnimation(nullptr)
    , currentFrameIndex(0)
    , frameTimer(0.0f)
    , playing(false)
    , paused(false)
    , finished(false)
{}

void AnimatedSprite::setTexture(sf::Texture* tex) {
    texture = tex;
    if (texture) {
        sprite.setTexture(*texture);
    }
}

void AnimatedSprite::addAnimation(const Animation& animation) {
    animations[animation.name] = animation;
}

void AnimatedSprite::play(const std::string& animationName, bool forceRestart) {
    // Check if animation exists
    auto it = animations.find(animationName);
    if (it == animations.end()) {
        Logger::warning("Animation '" + animationName + "' not found");
        return;
    }

    // If same animation and already playing, don't restart unless forced
    if (currentAnimationName == animationName && playing && !forceRestart) {
        return;
    }

    // Set new animation
    currentAnimationName = animationName;
    currentAnimation = &it->second;
    currentFrameIndex = 0;
    frameTimer = 0.0f;
    playing = true;
    paused = false;
    finished = false;

    // Set first frame
    if (!currentAnimation->frames.empty()) {
        sprite.setTextureRect(currentAnimation->frames[0]);
    }
}

void AnimatedSprite::stop() {
    playing = false;
    paused = false;
    currentFrameIndex = 0;
    frameTimer = 0.0f;
}

void AnimatedSprite::pause() {
    paused = true;
}

void AnimatedSprite::resume() {
    paused = false;
}

void AnimatedSprite::update(float dt) {
    if (!playing || paused || !currentAnimation || currentAnimation->frames.empty()) {
        return;
    }

    // Update timer
    frameTimer += dt;

    // Check if we should advance to next frame
    if (frameTimer >= currentAnimation->frameDuration) {
        frameTimer -= currentAnimation->frameDuration;
        currentFrameIndex++;

        // Check if animation finished
        if (currentFrameIndex >= currentAnimation->frames.size()) {
            if (currentAnimation->loop) {
                // Loop back to start
                currentFrameIndex = 0;
            } else {
                // Stay on last frame
                currentFrameIndex = currentAnimation->frames.size() - 1;
                finished = true;
                playing = false;
            }
        }

        // Update sprite texture rect
        sprite.setTextureRect(currentAnimation->frames[currentFrameIndex]);
    }
}

void AnimatedSprite::draw(sf::RenderWindow& window) {
    window.draw(sprite);
}

void AnimatedSprite::setPosition(float x, float y) {
    sprite.setPosition(x, y);
}

void AnimatedSprite::setPosition(const sf::Vector2f& position) {
    sprite.setPosition(position);
}

sf::Vector2f AnimatedSprite::getPosition() const {
    return sprite.getPosition();
}

void AnimatedSprite::setScale(float x, float y) {
    sprite.setScale(x, y);
}

void AnimatedSprite::setScale(const sf::Vector2f& scale) {
    sprite.setScale(scale);
}

void AnimatedSprite::setOrigin(float x, float y) {
    sprite.setOrigin(x, y);
}

void AnimatedSprite::setOrigin(const sf::Vector2f& origin) {
    sprite.setOrigin(origin);
}

void AnimatedSprite::setRotation(float angle) {
    sprite.setRotation(angle);
}

void AnimatedSprite::setColor(const sf::Color& color) {
    sprite.setColor(color);
}

std::string AnimatedSprite::getCurrentAnimation() const {
    return currentAnimationName;
}

bool AnimatedSprite::isFinished() const {
    return finished;
}

sf::FloatRect AnimatedSprite::getGlobalBounds() const {
    return sprite.getGlobalBounds();
}

sf::FloatRect AnimatedSprite::getLocalBounds() const {
    return sprite.getLocalBounds();
}

