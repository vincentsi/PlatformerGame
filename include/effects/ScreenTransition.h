#pragma once
#include <SFML/Graphics.hpp>

class ScreenTransition {
public:
    enum class State {
        None,
        FadingOut,
        FadingIn
    };

    ScreenTransition();

    void startFadeOut(float duration = 0.5f);
    void startFadeIn(float duration = 0.5f);

    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool isComplete() const { return state == State::None; }
    bool isFadedOut() const { return state == State::FadingOut && alpha >= 255.0f; }
    State getState() const { return state; }

private:
    State state;
    float alpha;
    float fadeSpeed;
    sf::RectangleShape overlay;
};
